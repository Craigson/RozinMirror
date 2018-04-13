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
    
    RozinMirror();
    RozinMirror(int cols, int rows, int scale);
    
    void update();
    void render();
    
    
private:
    
    int mNumX, mNumY, mScale, mScaleX, mScaleY;
    
    ci::gl::BatchRef            mInstanceBatch;
    ci::gl::TextureRef          mTexture;
    ci::gl::GlslProgRef         mGlsl;
    ci::gl::VboRef              mInstanceDataVbo;

    
    
};
