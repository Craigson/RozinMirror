//
//  RozinMirror.hpp
//  RozinMirror
//
//  Created by Craig Pickard on 4/13/18.
//

#pragma once

typedef std::shared_ptr<class RozinMirror> RozinMirrorRef;

class RozinMirror {
    
public:
    static RozinMirrorRef create(){ return RozinMirrorRef(new RozinMirror());}
    static RozinMirrorRef create(int _x, int _y, int _c){ return RozinMirrorRef(new RozinMirror(_x, _y, _c));}
    
    RozinMirror();
    RozinMirror(int cols, int rows, int scale);
    
    void update();
    void update(const std::vector<float> &mRotations);
    void render();
    
    inline int getNumX(){ return mNumX; }
    inline int getNumY(){ return mNumY; }
    
    
private:
    
    int mNumX, mNumY, mScale, mScaleX, mScaleY;
    
    ci::gl::BatchRef            mInstanceBatch;
    ci::gl::TextureRef          mTexture;
    ci::gl::GlslProgRef         mGlsl;
    ci::gl::VboRef              mInstanceDataVbo;

    
    
};
