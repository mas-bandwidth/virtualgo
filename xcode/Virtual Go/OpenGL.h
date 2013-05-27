//
//  OpenGL.h
//  Virtual Go
//
//  Created by Glenn Fiedler on 5/8/13.
//  Copyright (c) 2013 Glenn Fiedler. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import "Mesh.h"

@interface OpenGL : NSObject

- (GLuint)loadShader:(NSString*)filename;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file;
- (BOOL)linkProgram:(GLuint)prog;
- (BOOL)validateProgram:(GLuint)prog;

- (GLuint)loadTexture:(NSString *)filename;

- (void)destroyBuffer:(GLuint&)buffer;
- (void)destroyProgram:(GLuint&)program;
- (void)destroyTexture:(GLuint&)texture;
- (void)destroyFramebuffer:(GLuint&)framebuffer;

- (void) generateVBAndIBFromMesh:(Mesh<Vertex>&) mesh
                    vertexBuffer:(GLuint&) vb
                     indexBuffer:(GLuint&) ib
               vertexArrayObject:(GLuint&) vao;

- (void) generateVBAndIBFromTexturedMesh:(Mesh<TexturedVertex>&) mesh
                            vertexBuffer:(GLuint&) vb
                             indexBuffer:(GLuint&) ib
                       vertexArrayObject:(GLuint&) vao;

@end
