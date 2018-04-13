//
//  RozinMirror.cpp
//  RozinMirror
//
//  Created by Craig Pickard on 4/13/18.
//

#include "RozinMirror.hpp"

RozinMirror::RozinMirror()
:mNumX(64),mNumY(48),mScale(10)
{
    mScaleX = mNumX*mScale;
    mScaleY = mNumY*mScale;
    
    // Shader for the 3D instanced object
    mGlsl = ci::gl::GlslProg::create( ci::app::loadAsset( "shader.vert" ), ci::app::loadAsset( "shader.frag" ) );
    
    ci::gl::VboMeshRef mesh = ci::gl::VboMesh::create( ci::geom::Cylinder() );
    
    // create an array of initial per-instance positions laid out in a 2D grid
    std::vector<ci::mat4> transforms;
    
    for( size_t potX = 0; potX < mNumX; ++potX ) {
        for( size_t potY = 0; potY < mNumY; ++potY ) {
            float instanceX = potX / (float)mNumX - 0.5f;
            float instanceY = potY / (float)mNumY - 0.5f;
            ci::mat4 newMat;
            newMat = glm::translate( ci::vec3( instanceX * mScaleX, instanceY * mScaleY, 0));
            //            newMat *= glm::scale( vec3(5.0f, 1.0f, 5.f) );
            transforms.push_back( newMat );
        }
    }
    
    // create the VBO which will contain per-instance (rather than per-vertex) data
    mInstanceDataVbo = ci::gl::Vbo::create( GL_ARRAY_BUFFER, transforms.size() * sizeof(ci::mat4), transforms.data(), GL_DYNAMIC_DRAW );
    
    // we need a geom::BufferLayout to describe this data as mapping to the CUSTOM_0 semantic, and the 1 (rather than 0) as the last param indicates per-instance (rather than per-vertex)
    ci::geom::BufferLayout instanceDataLayout;
    instanceDataLayout.append( ci::geom::Attrib::CUSTOM_0, 4, sizeof(ci::mat4), 0, 1 /* per instance */ );
    
    // now add it to the VboMesh we already made of the Teapot
    mesh->appendVbo( instanceDataLayout, mInstanceDataVbo );
    
    // and finally, build our batch, mapping our CUSTOM_0 attribute to the "vInstancePosition" GLSL vertex attribute
    mInstanceBatch = ci::gl::Batch::create( mesh, mGlsl, { { ci::geom::Attrib::CUSTOM_0, "vInstanceMatrix" } } );
    
    
    mTexture = ci::gl::Texture::create( loadImage( ci::app::loadAsset( "texture.jpg" ) ), ci::gl::Texture::Format().mipmap() );
    mTexture->bind();
    
    // Shader for the 3D instanced object
    mGlsl = ci::gl::GlslProg::create( ci::app::loadAsset( "shader.vert" ), ci::app::loadAsset( "shader.frag" ) );
    
    // load the texture
    mTexture = ci::gl::Texture::create( loadImage( ci::app::loadAsset( "texture.jpg" ) ), ci::gl::Texture::Format().mipmap() );
    mTexture->bind();
    

}


RozinMirror::RozinMirror(int cols, int rows, int scale)
:mNumX(cols),mNumY(rows),mScale(scale)
{
    mScaleX = mNumX/mScale;
    mScaleY = mNumY/mScale;
    
    // Shader for the 3D instanced object
    mGlsl = ci::gl::GlslProg::create( ci::app::loadAsset( "shader.vert" ), ci::app::loadAsset( "shader.frag" ) );
    
    ci::gl::VboMeshRef mesh = ci::gl::VboMesh::create( ci::geom::Cylinder() );
    
    // create an array of initial per-instance positions laid out in a 2D grid
    std::vector<ci::mat4> transforms;
    
    for( size_t potX = 0; potX < mNumX; ++potX ) {
        for( size_t potY = 0; potY < mNumY; ++potY ) {
            float instanceX = potX / (float)mNumX - 0.5f;
            float instanceY = potY / (float)mNumY - 0.5f;
            ci::mat4 newMat;
            newMat = glm::translate( ci::vec3( instanceX * mScaleX, instanceY * mScaleY, 0));
            //            newMat *= glm::scale( vec3(5.0f, 1.0f, 5.f) );
            transforms.push_back( newMat );
        }
    }
    
    // create the VBO which will contain per-instance (rather than per-vertex) data
    mInstanceDataVbo = ci::gl::Vbo::create( GL_ARRAY_BUFFER, transforms.size() * sizeof(ci::mat4), transforms.data(), GL_DYNAMIC_DRAW );
    
    // we need a geom::BufferLayout to describe this data as mapping to the CUSTOM_0 semantic, and the 1 (rather than 0) as the last param indicates per-instance (rather than per-vertex)
    ci::geom::BufferLayout instanceDataLayout;
    instanceDataLayout.append( ci::geom::Attrib::CUSTOM_0, 4, sizeof(ci::mat4), 0, 1 /* per instance */ );
    
    // now add it to the VboMesh we already made of the Teapot
    mesh->appendVbo( instanceDataLayout, mInstanceDataVbo );
    
    // and finally, build our batch, mapping our CUSTOM_0 attribute to the "vInstancePosition" GLSL vertex attribute
    mInstanceBatch = ci::gl::Batch::create( mesh, mGlsl, { { ci::geom::Attrib::CUSTOM_0, "vInstanceMatrix" } } );
    
    // load the texture
    mTexture = ci::gl::Texture::create( loadImage( ci::app::loadAsset( "texture.jpg" ) ), ci::gl::Texture::Format().mipmap() );
    mTexture->bind();
    
  
}


void RozinMirror::update()
{
    // update our instance positions; map our instance data VBO, write new positions, unmap
    ci::mat4 *transforms = (ci::mat4*)mInstanceDataVbo->mapReplace();
    
    for( size_t potX = 0; potX < mNumX; ++potX ) {
        for( size_t potY = 0; potY < mNumY; ++potY ) {
            
            float instanceX = potX / (float)mNumX - 0.5f;
            float instanceY = potY / (float)mNumY - 0.5f;
            
            ci::mat4 newMat;
            float rot = sin( potX * potY * ci::app::getElapsedSeconds()/5000) * 180;
            
            //            float rot = 1;
            
            newMat *= glm::translate( ci::vec3( instanceX * mScaleX, instanceY * mScaleY, 0));
            newMat *= glm::rotate(  glm::radians(rot) ,ci::vec3(1,0,0) );
            newMat *= glm::scale( ci::vec3(5.0f, 1.0f, 5.f) );
            
            *transforms++ = newMat;
        }
    }
    mInstanceDataVbo->unmap();
}

void RozinMirror::update(const std::vector<float> &mRotations)
{
    // update our instance positions; map our instance data VBO, write new positions, unmap
    ci::mat4 *transforms = (ci::mat4*)mInstanceDataVbo->mapReplace();
    
    int index = 0;
    
    for( size_t potX = 0; potX < mNumX; ++potX ) {
        for( size_t potY = 0; potY < mNumY; ++potY ) {
            
            float instanceX = potX / (float)mNumX - 0.5f;
            float instanceY = potY / (float)mNumY - 0.5f;
            
            ci::mat4 newMat;
            float rot = ci::lmap(mRotations[index], -1.f, 1.f, 0.f, 90.f);
            
            newMat *= glm::translate( ci::vec3( instanceX * mScaleX, instanceY * mScaleY, 0));
            newMat *= glm::rotate(  glm::radians(rot) ,ci::vec3(1,0,0) );
            newMat *= glm::scale( ci::vec3(5.0f, 1.0f, 5.f) );
            
            *transforms++ = newMat;
            index++;
        }
    }
    mInstanceDataVbo->unmap();
}

void RozinMirror::render()
{
    mInstanceBatch->drawInstanced( mNumX * mNumY );
}
