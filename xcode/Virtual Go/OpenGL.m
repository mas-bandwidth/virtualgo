#import "OpenGL.h"

@implementation OpenGL

- (id) init
{
    if ( self = [super init] )
    {
        // ...
    }
    return self;
}

- (GLuint)loadShader:(NSString*)filename
{
    GLuint vertShader, fragShader;

    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    GLuint program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:filename ofType:@"vertex"];
    
    if ( !vertShaderPathname )
    {
        NSLog( @"Could not find vertex shader: %@", filename );
        return 0;
    }

    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname])
    {
        NSLog( @"Failed to compile vertex shader" );
        return 0;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:filename ofType:@"fragment"];

    if ( !fragShaderPathname )
    {
        NSLog( @"Could not find fragment shader: %@", filename );
        return 0;
    }

    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname])
    {
        NSLog( @"Failed to compile fragment shader" );
        return 0;
    }
    
    // Attach vertex shader to program.
    glAttachShader( program, vertShader );
    
    // Attach fragment shader to program.
    glAttachShader( program, fragShader );
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation( program, GLKVertexAttribPosition, "position" );
    glBindAttribLocation( program, GLKVertexAttribNormal, "normal" );
    glBindAttribLocation( program, GLKVertexAttribTexCoord0, "texCoords" );
    
    // Link program.
    if (![self linkProgram:program])
    {
        NSLog( @"Failed to link program: %d", program );
        
        if ( vertShader )
        {
            glDeleteShader( vertShader );
            vertShader = 0;
        }
        if ( fragShader )
        {
            glDeleteShader( fragShader );
            fragShader = 0;
        }
        if ( program )
            glDeleteProgram( program );
        
        return 0;
    }
        
    // Release vertex and fragment shaders.
    if ( vertShader )
    {
        glDetachShader( program, vertShader );
        glDeleteShader( vertShader );
    }
    if ( fragShader )
    {
        glDetachShader( program, fragShader );
        glDeleteShader( fragShader );
    }
    
    return program;
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source)
    {
        NSLog( @"Failed to load vertex shader" );
        return NO;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv( *shader, GL_INFO_LOG_LENGTH, &logLength );
    if ( logLength > 0 )
    {
        GLchar *log = (GLchar *)malloc( logLength );
        glGetShaderInfoLog( *shader, logLength, &logLength, log );
        NSLog( @"Shader compile log:\n%s", log );
        free( log );
    }
#endif
    
    glGetShaderiv( *shader, GL_COMPILE_STATUS, &status );
    if ( status == 0 )
    {
        glDeleteShader( *shader );
        return NO;
    }
    
    return YES;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    glLinkProgram( prog );
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv( prog, GL_INFO_LOG_LENGTH, &logLength );
    if ( logLength > 0 )
    {
        GLchar *log = (GLchar *)malloc( logLength );
        glGetProgramInfoLog( prog, logLength, &logLength, log );
        NSLog( @"Program link log:\n%s", log );
        free( log );
    }
#endif
    
    glGetProgramiv( prog, GL_LINK_STATUS, &status );
    if ( status == 0 )
    {
        return NO;
    }
    
    return YES;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;
    
    glValidateProgram( prog );
    glGetProgramiv( prog, GL_INFO_LOG_LENGTH, &logLength );
    if ( logLength > 0 )
    {
        GLchar *log = (GLchar *)malloc( logLength );
        glGetProgramInfoLog( prog, logLength, &logLength, log );
        NSLog( @"Program validate log:\n%s", log );
        free(log);
    }
    
    glGetProgramiv( prog, GL_VALIDATE_STATUS, &status );
    if ( status == 0 )
    {
        return NO;
    }
    
    return YES;
}

- (GLuint)loadTexture:(NSString *)filename
{
    // todo: really should generate texture mipchain using an offline tool
    // then just load that already baked out texture here. this increases load time
    // and will not scale when lots of textures need to be loaded for different boards
    // and different sets of stones
    
    CGImageRef textureImage = [UIImage imageNamed:filename].CGImage;
    if ( !textureImage )
    {
        NSLog( @"Failed to load image %@", filename );
        exit(1);
    }
 
    size_t textureWidth = CGImageGetWidth( textureImage );
    size_t textureHeight = CGImageGetHeight( textureImage );
 
    GLubyte * textureData = (GLubyte *) calloc( textureWidth*textureHeight*4, sizeof(GLubyte) );
 
    CGContextRef textureContext = CGBitmapContextCreate( textureData, textureWidth, textureHeight, 8, textureWidth*4,
        CGImageGetColorSpace(textureImage), kCGImageAlphaPremultipliedLast );
 
    CGContextDrawImage( textureContext, CGRectMake( 0, 0, textureWidth, textureHeight ), textureImage );
 
    CGContextRelease( textureContext );
 
    GLuint textureId;

    glGenTextures( 1, &textureId );
    glBindTexture( GL_TEXTURE_2D, textureId );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData );

    glHint( GL_GENERATE_MIPMAP_HINT, GL_NICEST );
    glGenerateMipmap( GL_TEXTURE_2D );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    
    free( textureData );

    return textureId;
}

- (void)destroyBuffer:(GLuint&)buffer
{
    if ( buffer )
    {
        glDeleteBuffers( 1, &buffer );
        buffer = 0;
    }
}

- (void)destroyProgram:(GLuint&)program
{
    if ( program )
    {
        glDeleteBuffers( 1, &program );
        program = 0;
    }
}

- (void)destroyTexture:(GLuint&)texture
{
    if ( texture )
    {
        glDeleteTextures( 1, &texture );
        texture = 0;
    }
}

- (void) generateVBAndIBFromMesh:(Mesh<Vertex>&) mesh
                    vertexBuffer:(GLuint&) vb
                     indexBuffer:(GLuint&) ib
{
    void * vertexData = mesh.GetVertexBuffer();
    
    assert( vertexData );
    assert( mesh.GetNumIndices() > 0 );
    assert( mesh.GetNumVertices() > 0 );
    
    glGenBuffers( 1, &vb );
    glBindBuffer( GL_ARRAY_BUFFER, vb );
    glBufferData( GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.GetNumVertices(), vertexData, GL_STATIC_DRAW );
    
    glGenBuffers( 1, &ib );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ib );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, mesh.GetNumIndices()*sizeof(GLushort), mesh.GetIndexBuffer(), GL_STATIC_DRAW );
}

- (void) generateVBAndIBFromTexturedMesh:(Mesh<TexturedVertex>&) mesh
                            vertexBuffer:(GLuint&) vb
                             indexBuffer:(GLuint&) ib
{
    void * vertexData = mesh.GetVertexBuffer();
    
    assert( vertexData );
    assert( mesh.GetNumIndices() > 0 );
    assert( mesh.GetNumVertices() > 0 );
    
    glGenBuffers( 1, &vb );
    glBindBuffer( GL_ARRAY_BUFFER, vb );
    glBufferData( GL_ARRAY_BUFFER, sizeof(TexturedVertex) * mesh.GetNumVertices(), vertexData, GL_STATIC_DRAW );
    
    glGenBuffers( 1, &ib );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ib );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, mesh.GetNumIndices()*sizeof(GLushort), mesh.GetIndexBuffer(), GL_STATIC_DRAW );
}

- (void)selectTexturedMesh:(GLuint)texture vertexBuffer:(GLuint)vb indexBuffer:(GLuint)ib
{
    glBindTexture( GL_TEXTURE_2D, texture );
    
    glBindBuffer( GL_ARRAY_BUFFER, vb );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ib );
    
    glEnableVertexAttribArray( GLKVertexAttribPosition );
    glEnableVertexAttribArray( GLKVertexAttribNormal );
    glEnableVertexAttribArray( GLKVertexAttribTexCoord0 );
    
    glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), 0 );
    glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)16 );
    glVertexAttribPointer( GLKVertexAttribTexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)32 );
}

- (void)selectNonTexturedMesh:(GLuint)vb indexBuffer:(GLuint)ib
{
    glBindTexture( GL_TEXTURE_2D, 0 );
    
    glBindBuffer( GL_ARRAY_BUFFER, vb );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ib );
    
    glEnableVertexAttribArray( GLKVertexAttribPosition );
    glEnableVertexAttribArray( GLKVertexAttribNormal );
    glDisableVertexAttribArray( GLKVertexAttribTexCoord0 );
    
    glVertexAttribPointer( GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0 );
    glVertexAttribPointer( GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)16 );
}

@end
