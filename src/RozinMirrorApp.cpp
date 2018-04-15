#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/Capture.h"
#include "cinder/Log.h"
#include "cinder/ImageIo.h"
#include "cinder/Thread.h"
#include "cinder/ConcurrentCircularBuffer.h"
#include "cinder/gl/Sync.h"
#include "cinder/gl/Context.h"
#include "cinder/ObjLoader.h"
#include "cinder/Utilities.h"
#include "cinder/params/Params.h"

#include "POV.hpp"
#include "RozinMirror.hpp"


//#define TILE_SIZE 4
#define CAM_RESOLUTION_X 640
#define CAM_RESOLUTION_Y 480
#define COUNT_X 128
#define COUNT_Y 96

const int TILE_SIZE = CAM_RESOLUTION_X / COUNT_X;

using namespace ci;
using namespace ci::app;
using namespace std;

class RozinMirrorApp : public App {
  public:
    ~RozinMirrorApp(); // we need destructor for threading
	void setup() override;
	void mouseDown( MouseEvent event ) override;
    void mouseWheel( MouseEvent event ) override;
    void keyDown (KeyEvent event ) override;
    
	void update() override;
	void draw() override;
    void initParams();
    void create3Dgrid();
    void updateGrid();
    void initThread();
    void updateCapture();
    void renderCapture();
    void calculateDifferences();
    
    // Camera System
     POV                            mPov;
    
    // Params
    params::InterfaceGlRef         mParams;
    string                         mFps;
    bool                           mShowFps;
    bool                           mShowCapture;
    
    // RozinMirror Grid
    RozinMirrorRef                  mRozinMirror;
    
    // Threading
    void captureImagesThreadFn ( gl::ContextRef sharedGlContext );
    
    ConcurrentCircularBuffer<ci::Surface8uRef>      *mCaptureFrames, *mTiledFrames;
    shared_ptr<thread>                              mThread;
    bool                                            mShouldQuit;
    double                                          mLastTime;
    
    // Capture
    CaptureRef            mCapture;
    gl::TextureRef        mTexture, mTiledTexture, mReferenceTexture, mTiledReferenceTexture;
    ci::Surface8uRef      mCamSurface, mTiledSurface, mTiledReferenceSurface;
    
    bool                  mReferenceIsSet;
    
    std::vector<float>      mDifferenceValues; // a container for storing the values of the difference between the reference frame and the current frame
    
};

void RozinMirrorApp::setup()
{
    initParams();
    
    // Create the camera controller.
    mPov = POV( this, ci::vec3( 0.0f, 0.0f, 3000.0f ), ci::vec3( 0.0f, 0.0f, 0.0f ) );
    
    // create hte rozin mirror grid
    mRozinMirror = RozinMirror::create(COUNT_X,COUNT_Y,20);
    
    // init capture
    try {
        mCapture = Capture::create( CAM_RESOLUTION_X, CAM_RESOLUTION_Y );
        mCapture->start();
    }
    catch( ci::Exception &exc ) {
        CI_LOG_EXCEPTION( "Failed to init capture ", exc );
    }
    
    mReferenceIsSet = false;
    
    mReferenceTexture = ci::gl::Texture::create(CAM_RESOLUTION_X, CAM_RESOLUTION_Y);
    mTiledReferenceTexture = ci::gl::Texture::create(CAM_RESOLUTION_X, CAM_RESOLUTION_Y);
    
    mTiledReferenceSurface = ci::Surface8u::create(CAM_RESOLUTION_X, CAM_RESOLUTION_Y, false);
    
    // init the thread
    initThread();
    
    mDifferenceValues.resize(mRozinMirror->getNumX() * mRozinMirror->getNumY() );
    for (auto i : mDifferenceValues) i = 0.0f;
    
    // General
    gl::enableDepthWrite();
    gl::enableDepthRead();
    
}

void RozinMirrorApp::mouseDown( MouseEvent event )
{
    cout << "Updating reference Images" << std::endl;
    
    if (mCamSurface) mReferenceTexture->update( *mCamSurface );
//    if (mTiledSurface) {
//        mTiledReferenceTexture->update( *mTiledSurface );
//        mTiledReferenceSurface = mTiledSurface;
//        mReferenceIsSet = true;
//    }
    
    
}

void RozinMirrorApp::mouseWheel( MouseEvent event )
{
     mPov.adjustDist( event.getWheelIncrement() * -5.0f );
}

void RozinMirrorApp::keyDown( KeyEvent event)
{
    switch (event.getChar()){
        case 'i':
            mPov.adjustDist(-1500);
            break;
            
        case 'o':
            mPov.adjustDist(1500);
            break;
            
        default:
            
            break;
    }
}

void RozinMirrorApp::update()
{
    // update the camera controller
    mPov.update();
    
    // update the rozin mirror
//    mRozinMirror->update();
    mRozinMirror->update(mDifferenceValues);
    
    // update the capture
    updateCapture();
    
    if (mTiledSurface) {
        mTiledReferenceTexture->update( *mTiledSurface );
        mTiledReferenceSurface = mTiledSurface;
        calculateDifferences();
    }
    
    if (mShowFps) mFps = toString(int(getAverageFps()));
}

void RozinMirrorApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
    
    gl::setMatrices( mPov.mCam);
    mRozinMirror->render();
    
    // draw the capture
    if(mShowCapture) renderCapture();
    
    // Draw the interface
    mParams->draw();
}

// Destructor
RozinMirrorApp::~RozinMirrorApp()
{
    mShouldQuit = true;
    mThread->join();
    mCaptureFrames->cancel();
    mTiledFrames->cancel();
}

/********************* PARAMS ************************/

void RozinMirrorApp::initParams()
{
    // Params
    mParams = params::InterfaceGl::create( getWindow(), "App parameters", toPixels( ivec2( 180, 80 ) ) );
    mShowFps = true;
    mParams->addParam("Show FPS", &mShowFps);
    mParams->addParam( "Fps", &mFps );
    
    mParams->addSeparator();
    
    mShowCapture = false;
    mParams->addParam("Show Capture", &mShowCapture);
}


/********************* THREADING ************************/

void RozinMirrorApp::initThread()
{
    mShouldQuit = false;
    mCaptureFrames = new ConcurrentCircularBuffer<ci::Surface8uRef>( 5 ); // room for 5 images
    mTiledFrames = new ConcurrentCircularBuffer<ci::Surface8uRef>( 5 ); // room for 5 images
    
    // create and launch the thread with a new gl::Context just for that thread
    gl::ContextRef backgroundCtx = gl::Context::create( gl::context() );
    
    mThread = shared_ptr<thread>( new thread( bind( &RozinMirrorApp::captureImagesThreadFn, this, backgroundCtx ) ) );
    

}


void RozinMirrorApp::captureImagesThreadFn(gl::ContextRef context)
{
    ci::ThreadSetup threadSetup; // instantiate this if you're talking to Cinder from a secondary thread
    // we received as a parameter a gl::Context we can use safely that shares resources with the primary Context
    context->makeCurrent();
    
    // load images as Textures into our ConcurrentCircularBuffer
    while(( ! mShouldQuit ))  {
        
        if( mCapture && mCapture->checkNewFrame() ) {
            
            if (!mTiledSurface) {
                cout << "creating tiled surface" << std::endl;
                mTiledSurface = ci::Surface8u::create(mCapture->getWidth(), mCapture->getHeight(), false);
            }
            
            auto outputSurf = ci::Surface8u::create(mCapture->getWidth(), mCapture->getHeight(), false);
            
            auto capturedSurface = mCapture->getSurface();
            
//            int w = (*mCapture->getSurface()).getWidth();
//            int h = (*mCapture->getSurface()).getHeight();
            
            // sum the values of an area (determined by TILE_SIZE) and created  a "pixelated" version of the input image by taking the average for each tile
            for (size_t i = 0; i < COUNT_X; i++ ) {
                for (size_t j = 0; j < COUNT_Y; j++ ) {
                    
                    float r = 0;
                    float g = 0;
                    float b = 0;
                    
                    Surface::Iter outputIter = outputSurf->getIter( ci::Area(i*TILE_SIZE,j*TILE_SIZE,i*TILE_SIZE+TILE_SIZE,j*TILE_SIZE+TILE_SIZE));
                    
                    Surface::ConstIter captureIter( capturedSurface->getIter( ci::Area(i*TILE_SIZE,j*TILE_SIZE,i*TILE_SIZE+TILE_SIZE,j*TILE_SIZE+TILE_SIZE)));
                    
                    while(captureIter.line() ) {
                        while(captureIter.pixel() ) {
                            r += captureIter.r();
                            g += captureIter.g();
                            b += captureIter.b();
                        }
                    }
                    
                    int n = TILE_SIZE*TILE_SIZE;
                    
                    float avgR = r / n;
                    float avgG = r / n;
                    float avgB = r / n;
                    
                    //                    float avg = (avgR + avgG + avgB) / 3;
                    
                    while(outputIter.line() ) {
                        while(outputIter.pixel() ) {
                            outputIter.r() = avgR;
                            outputIter.g() = avgG;
                            outputIter.b() = avgB;
                        }
                    }
                }
            }
            
            
            // we need to wait on a fence before alerting the primary thread that the surface is ready
            auto fence = gl::Sync::create();
            fence->clientWaitSync();
            
            mCaptureFrames->pushFront(ci::Surface8u::create( *mCapture->getSurface() ));
            mTiledFrames->pushFront(outputSurf);
        }
    }
}


/********************* CAPTURE ************************/

void RozinMirrorApp::updateCapture()
{
    if (mCaptureFrames->isNotEmpty())
    {
        mCaptureFrames->popBack(&mCamSurface);
    }
    
    if (mTiledFrames->isNotEmpty())
    {
        mTiledFrames->popBack(&mTiledSurface);
    }
    
    if (mCamSurface)
    {
        if (!mTexture) {
            mTexture = ci::gl::Texture::create( *mCamSurface ); // this can be elimnated by placing in setup
        } else {
            mTexture->update(*mCamSurface);
        }
    }
    
    if (mTiledSurface)
    {
        if (!mTiledTexture) {
            mTiledTexture = ci::gl::Texture::create( *mTiledSurface );
        } else {
            mTiledTexture->update(*mTiledSurface);
        }
    }
}


void RozinMirrorApp::renderCapture()
{
    gl::ScopedMatrices scopeMtrx;
    gl::setMatricesWindow(getWindowSize());
    
    
    // draw the camera image
    if( mTexture ) {
        gl::ScopedModelMatrix modelScope;
        gl::draw( mTexture, ci::Rectf(0,0,mTexture->getWidth(), mTexture->getHeight()));
    }
    
    // draw the averaged image
    if (mTiledTexture && mTexture) {
        gl::ScopedModelMatrix modelScope;
        gl::draw( mTiledTexture, ci::Rectf(mTexture->getWidth(), 0, mTexture->getWidth() + mTiledTexture->getWidth(), mTiledTexture->getHeight()));
    }
    
    // draw the reference image
    if (mReferenceTexture && mTexture)
    {
        gl::ScopedModelMatrix modelScope;
        gl::draw( mReferenceTexture, ci::Rectf(0, mTexture->getHeight(), mReferenceTexture->getWidth(), mTexture->getHeight() + mReferenceTexture->getHeight()));
    }
    
    // draw the averaged reference image
    if (mTiledReferenceTexture && mTexture)
    {
        gl::ScopedModelMatrix modelScope;
        gl::draw( mTiledReferenceTexture, ci::Rectf(mTexture->getWidth(), mTexture->getHeight(), mTexture->getWidth() + mTiledReferenceTexture->getWidth(), mTexture->getHeight() + mTiledReferenceTexture->getHeight()));
    }
    
}

void RozinMirrorApp::calculateDifferences()
{
    Surface::ConstIter referenceFrameIter( mTiledReferenceSurface->getIter() );
    
    Surface::ConstIter currentFrameIter( mTiledSurface->getIter() );
    
    int index = 0;
    
    for ( size_t i = 0; i < mTiledSurface->getWidth(); i += TILE_SIZE){
        for (size_t j = 0; j < mTiledSurface->getHeight(); j += TILE_SIZE){
            // samle colour from the center of the tiles
//            ColorAf refColour = mTiledReferenceSurface->getPixel( ci::ivec2(i+(TILE_SIZE/2), j+(TILE_SIZE/2)) );
            ColorAf currentColour = mTiledSurface->getPixel( ci::ivec2(i+(TILE_SIZE/2), j+(TILE_SIZE/2)) );
            
//            float refAvg = (refColour.r + refColour.g + refColour.b)/3;
            float currentAvg = (currentColour.r + currentColour.g + currentColour.b)/3;
            
//            float diff = currentAvg - refAvg;
            float diff = 1.0 - currentAvg;
            
            mDifferenceValues[index] = diff;
            
            index++;
            
        }
    }
}


CINDER_APP( RozinMirrorApp, RendererGl, [&](App::Settings *settings){
    settings->setWindowSize(1920, 1080);
    settings->setHighDensityDisplayEnabled();
});
