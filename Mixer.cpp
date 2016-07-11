/* OpenGL example code - transform feedback
 *
 * This example simulates the same particle system as the buffer mapping
 * example. Instead of updating particles on the cpu and uploading
 * the update is done on the gpu with transform feedback.
 *
 * Autor: Jakob Progsch
 */

#include <GLXW/glxw.h>
#include <GLFW/glfw3.h>

//glm is used to create perspective and transform matrices
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

#include <fmod.hpp>
#include <fmod_errors.h>

#include "boost/filesystem.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <cmath>

#define SOUND_CLIP_LENGTH_SECONDS 16
#define MIN_LOOP_LENGTH_SECONDS 4
#define MAXIMUM_LOOP_ITERATIONS 1
#define MAXIMUM_NUMBER_OF_CONCURRENT_LOOPS 1

enum class LoopType
{
  Interesting = 0,
  High,
  Mid,
  Low,
  Beat,
  size // Used as a cheap way to limit loops
};

const std::vector< std::string > kLoopTypeStrings =
{
  "Interesting",
  "High",
  "Mid",
  "Low",
  "Beat",
};

const int kLoopTypeLimit[(int)LoopType::size] = { 1, 1, 1, 1, 1 };
const float kLoopTypeRelativeVolume[(int)LoopType::size] = { 0.2, 0.3, 0.7, 0.8, 1.0 };

class BackgroundMusic
{
public:

  BackgroundMusic()
  {
    if ( FMOD::System_Create( &mpSystem ) != FMOD_OK ) {
      // Report Error
      return;
    }

    int driverCount = 0;
    mpSystem->getNumDrivers( &driverCount );

    if ( driverCount == 0 ) {
      // Report Error
      return;
    }

    mpSystem->getDSPBufferSize( &mDspBlockLength, 0 );

    // Adding a buffer for other sounds. Will need to be coordinated!!!
    mpSystem->init( MAXIMUM_NUMBER_OF_CONCURRENT_LOOPS+1, FMOD_INIT_NORMAL, NULL );

    // Load loops
    int index = 0;
    for( auto typeString : kLoopTypeStrings ) {
      auto samples = getFileList( std::string("C:\\Users\\fitzpatrick\\Dropbox\\DCS\\Music\\Background\\") + typeString );
      for( auto file : samples ) {
        createSound( (LoopType)index, file.c_str() );
        ++mLoopCount;
      }
      ++index;
    }

    mpSystem->createChannelGroup( "Background", &mChannelgroup );

    // Initialize our Instance with enough Channels
    std::cout << "Initializing mixer with " << mLoopCount << " files" << std::endl;
  }

  ~BackgroundMusic()
  {
    // Cleanup!
    //result = sound[NOTE_C]->release();
    //result = sound[NOTE_D]->release();
    //result = sound[NOTE_E]->release();

    //result = system->close();
    //result = system->release();
  }

  void start()
  {
    std::cout << "--------------------------------------------------------------------------" << std::endl;
    buildPlayList();
    std::cout << " " << std::endl;
  }

  void frame()
  {
    //std::size_t index = 0;
    //for ( auto& track : mActiveSounds ) {
    //  bool playing;
    //  track.channel->isPlaying( &playing );
    //  if ( !playing ) {
    //    std::cout << "Exit: " << track.name << std::endl;
    //    mActiveSounds.erase( mActiveSounds.begin() + index );
    //    buildPlayList();
    //    mLoadedSounds[track.type].push_back( track );
    //    break;
    //  }
    //  ++index;
    //}


    std::size_t index = 0;
    for ( auto track : mActiveSounds ) {
      unsigned long long dspclock;
      track.channel->getDSPClock( &dspclock, 0 );
      if ( dspclock > track.dspStop  ) {
        FMOD::Channel* chan = track.channel;
        chan->stop();

        mActiveSounds.erase( mActiveSounds.begin() + index );

        std::cout << dspclock 
          << "- Exit: " << track.name
          << "\n dif: " << dspclock - track.dspStop
          << std::endl;

        buildPlayList( );
        mLoadedSounds[track.type].push_back( track );
        break;
      }
      ++index;
    }

    //unsigned long long dspclock;
    //mChannelgroup->getDSPClock( &dspclock, 0 );
    //std::vector< TrackInfo > endedTracks;
    //std::vector< TrackInfo > validTracks;

    //for ( auto& track : mActiveSounds ) {
    //  if ( dspclock > track.dspStop ) {
    //    FMOD::Channel* chan = track.channel;
    //    chan->stop();
    //    std::cout << dspclock 
    //      << "- Exit: " << track.name
    //      << " dif: " << dspclock - track.dspStop
    //      << std::endl;
    //    endedTracks.push_back( track );
    //  } else {
    //    validTracks.push_back( track );
    //  }
    //}

    //if( endedTracks.size() ) {
    //  mActiveSounds = validTracks;
    //  buildPlayList();
    //  for( auto& track : endedTracks ) {
    //    mLoadedSounds[track.type].push_back( track );
    //  }
    //}

    // Sound system update
    mpSystem->update();
  }

  void createSound( LoopType type, std::string fileName )
  {
    FMOD_RESULT result;
    TrackInfo track;
    result = mpSystem->createStream( fileName.c_str(), FMOD_LOOP_NORMAL | FMOD_2D | FMOD_IGNORETAGS, 0, &track.sound );
    if ( result != FMOD_OK ) {
      std::cout << "*** ERROR ***\n" << FMOD_ErrorString( result ) << std::endl;
      return;
    }
    track.type = type;
    track.name = fileName;
    track.sound->addSyncPoint( 0, FMOD_TIMEUNIT_MS, "Start", 0 ); // Not sure this does anything

    unsigned int length;
    track.sound->getLength( &length, FMOD_TIMEUNIT_MS );
    std::cout << "Mixer loaded: " << track.name << " length: " << length <<std::endl;

    mLoadedSounds[type].push_back( track );
  }

  void buildPlayList()
  {
    int currentLoopTypeLimit[(int)LoopType::size] = { 0, 0, 0, 0, 0 };
    int currentChannelRandomCount = (rand()%MAXIMUM_NUMBER_OF_CONCURRENT_LOOPS)+1;

    for( auto& active : mActiveSounds ) {
      ++currentLoopTypeLimit[(int)active.type];
    }

    std::cout << "\n\nBuilding with " << currentChannelRandomCount << " channels..." << std::endl;

    while ( mActiveSounds.size() < currentChannelRandomCount ) {
      int randomType = rand() % (int)LoopType::size;
      if ( currentLoopTypeLimit[randomType] < kLoopTypeLimit[randomType] ) {
        addToMixer( 
          (LoopType)randomType, 
          rand() % mLoadedSounds[(LoopType)randomType].size(), 
          ( rand() % MAXIMUM_LOOP_ITERATIONS ) + 1,
          kLoopTypeRelativeVolume[randomType] );
      }
    }
  }

  void addToMixer( LoopType type, size_t index, size_t loopCount = 1, float soundLevel = 1.0 )
  {
    TrackInfo track = mLoadedSounds[type][index];

    track.sound->setMode( FMOD_LOOP_NORMAL );
    track.sound->setLoopCount( loopCount );
    unsigned int loopLength = MIN_LOOP_LENGTH_SECONDS * ((rand()%4)+1); // 1-4
    track.sound->setLoopPoints( 0, FMOD_TIMEUNIT_MS, loopLength * 1000, FMOD_TIMEUNIT_MS );
    //track.sound->seekData( TODO );
    mpSystem->playSound( track.sound, mChannelgroup, false, &track.channel );
    FMOD::Channel* chan = track.channel;
    //chan->setVolume( soundLevel );

    unsigned long long dspclock;
    FMOD::System *sys;
    int rate;

    chan->getSystemObject( &sys );
    sys->getSoftwareFormat( &rate, 0, 0 ); // Get mixer rate
    //mChannelgroup->getDSPClock( &dspclock, 0 );
    chan->getDSPClock( &dspclock, 0 );
    unsigned long long t0 = 0;
    unsigned long long t1 = rate * 5;
    unsigned long long t2 = rate * ( ( loopCount*loopLength ) - 2 );
    unsigned long long t3 = rate * ( loopCount*loopLength );

    track.dspStop = t3 + 1024;

    // Set fade points to create a ramp up and down for total channel duration
    chan->addFadePoint( dspclock + t0, 0.1f ); 
    chan->addFadePoint( dspclock + t1, soundLevel );
    chan->addFadePoint( dspclock + t2, soundLevel - 0.1f );
    chan->addFadePoint( dspclock + t3, 0.1f ); 

    mActiveSounds.push_back( track );
    mLoadedSounds[type].erase( mLoadedSounds[type].begin() + index );
    std::cout << dspclock << "- Playing loop: " << track.name 
      << "\n loopCount: " << loopCount
      << "\nloopLength: " << loopLength
      << "\n      rate: " << rate
      << "\n  Playtime: " << loopCount*loopLength
      << "\n     Start: " << dspclock
      << "\n    Length: " << t3
      << "\n       End: " << dspclock + t3
      << std::endl;
  }

  void releaseSound( FMOD::Sound* sound )
  {
    sound->release();
    //remove from mLoadedSounds
  }

private:
  struct TrackInfo
  {
    std::string name;
    LoopType type;
    FMOD::Sound* sound = 0;
    FMOD::Channel* channel = 0;
    unsigned long long dspStop = 0;
  };

  // Returns a list of file names given a folder
  std::vector<std::string> getFileList( const std::string& path )
  {
    std::vector<std::string> list;

    if ( !path.empty() ) {
      boost::filesystem::path apk_path( path );
      boost::filesystem::recursive_directory_iterator end;

      for ( boost::filesystem::recursive_directory_iterator i( apk_path ); i != end; ++i ) {
        const boost::filesystem::path cp = ( *i );
        list.push_back( cp.string() );
      }
    }
    return list;
  }

  // Pointer to the FMOD instance parameters
  FMOD::System *mpSystem = 0;
  FMOD::ChannelGroup *mChannelgroup = 0;
  unsigned int mDspBlockLength = 0;

  std::map< LoopType, std::vector< TrackInfo > > mLoadedSounds;
  unsigned int mLoopCount = 0;
  std::vector< TrackInfo > mActiveSounds;
};

//------------------------------------------------------------------------------------------------
// helper to check and display for shader compiler errors
bool check_shader_compile_status( GLuint obj )
{
  GLint status;
  glGetShaderiv( obj, GL_COMPILE_STATUS, &status );
  if ( status == GL_FALSE ) {
    GLint length;
    glGetShaderiv( obj, GL_INFO_LOG_LENGTH, &length );
    std::vector<char> log( length );
    glGetShaderInfoLog( obj, length, &length, &log[0] );
    std::cerr << &log[0];
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------------------------
// helper to check and display for shader linker error
bool check_program_link_status( GLuint obj )
{
  GLint status;
  glGetProgramiv( obj, GL_LINK_STATUS, &status );
  if ( status == GL_FALSE ) {
    GLint length;
    glGetProgramiv( obj, GL_INFO_LOG_LENGTH, &length );
    std::vector<char> log( length );
    glGetProgramInfoLog( obj, length, &length, &log[0] );
    std::cerr << &log[0];
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------------------------
int main()
{
  int width = 640;
  int height = 480;

  if ( glfwInit() == GL_FALSE ) {
    std::cerr << "failed to init GLFW" << std::endl;
    return 1;
  }

  // select opengl version
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );

  srand( time( NULL ) );

  // create a window
  GLFWwindow *window;
  if ( ( window = glfwCreateWindow( width, height, "09transform_feedback", 0, 0 ) ) == 0 ) {
    std::cerr << "failed to open window" << std::endl;
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent( window );

  if ( glxwInit() ) {
    std::cerr << "failed to init GL3W" << std::endl;
    glfwDestroyWindow( window );
    glfwTerminate();
    return 1;
  }

  // the vertex shader simply passes through data
  std::string vertex_source =
    "#version 330\n"
    "layout(location = 0) in vec4 vposition;\n"
    "void main() {\n"
    "   gl_Position = vposition;\n"
    "}\n";

  // the geometry shader creates the billboard quads
  std::string geometry_source =
    "#version 330\n"
    "uniform mat4 View;\n"
    "uniform mat4 Projection;\n"
    "layout (points) in;\n"
    "layout (triangle_strip, max_vertices = 4) out;\n"
    "out vec2 txcoord;\n"
    "void main() {\n"
    "   vec4 pos = View*gl_in[0].gl_Position;\n"
    "   txcoord = vec2(-1,-1);\n"
    "   gl_Position = Projection*(pos+0.2*vec4(txcoord,0,0));\n"
    "   EmitVertex();\n"
    "   txcoord = vec2( 1,-1);\n"
    "   gl_Position = Projection*(pos+0.2*vec4(txcoord,0,0));\n"
    "   EmitVertex();\n"
    "   txcoord = vec2(-1, 1);\n"
    "   gl_Position = Projection*(pos+0.2*vec4(txcoord,0,0));\n"
    "   EmitVertex();\n"
    "   txcoord = vec2( 1, 1);\n"
    "   gl_Position = Projection*(pos+0.2*vec4(txcoord,0,0));\n"
    "   EmitVertex();\n"
    "}\n";

  // the fragment shader creates a bell like radial color distribution    
  std::string fragment_source =
    "#version 330\n"
    "in vec2 txcoord;\n"
    "layout(location = 0) out vec4 FragColor;\n"
    "void main() {\n"
    "   float s = 0.2*(1/(1+15.*dot(txcoord, txcoord))-1/16.);\n"
    "   FragColor = s*vec4(0.3,0.3,1.0,1);\n"
    "}\n";

  // program and shader handles
  GLuint shader_program, vertex_shader, geometry_shader, fragment_shader;

  // we need these to properly pass the strings
  const char *source;
  int length;

  // create and compiler vertex shader
  vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  source = vertex_source.c_str();
  length = vertex_source.size();
  glShaderSource( vertex_shader, 1, &source, &length );
  glCompileShader( vertex_shader );
  if ( !check_shader_compile_status( vertex_shader ) ) {
    glfwDestroyWindow( window );
    glfwTerminate();
    return 1;
  }

  // create and compiler geometry shader
  geometry_shader = glCreateShader( GL_GEOMETRY_SHADER );
  source = geometry_source.c_str();
  length = geometry_source.size();
  glShaderSource( geometry_shader, 1, &source, &length );
  glCompileShader( geometry_shader );
  if ( !check_shader_compile_status( geometry_shader ) ) {
    glfwDestroyWindow( window );
    glfwTerminate();
    return 1;
  }

  // create and compiler fragment shader
  fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
  source = fragment_source.c_str();
  length = fragment_source.size();
  glShaderSource( fragment_shader, 1, &source, &length );
  glCompileShader( fragment_shader );
  if ( !check_shader_compile_status( fragment_shader ) ) {
    glfwDestroyWindow( window );
    glfwTerminate();
    return 1;
  }

  // create program
  shader_program = glCreateProgram();

  // attach shaders
  glAttachShader( shader_program, vertex_shader );
  glAttachShader( shader_program, geometry_shader );
  glAttachShader( shader_program, fragment_shader );

  // link the program and check for errors
  glLinkProgram( shader_program );
  check_program_link_status( shader_program );

  // obtain location of projection uniform
  GLint View_location = glGetUniformLocation( shader_program, "View" );
  GLint Projection_location = glGetUniformLocation( shader_program, "Projection" );



  // the transform feedback shader only has a vertex shader
  std::string transform_vertex_source =
    "#version 330\n"
    "uniform vec3 center[3];\n"
    "uniform float radius[3];\n"
    "uniform vec3 g;\n"
    "uniform float dt;\n"
    "uniform float bounce;\n"
    "uniform int seed;\n"
    "layout(location = 0) in vec3 inposition;\n"
    "layout(location = 1) in vec3 invelocity;\n"
    "out vec3 outposition;\n"
    "out vec3 outvelocity;\n"

    "float hash(int x) {\n"
    "   x = x*1235167 + gl_VertexID*948737 + seed*9284365;\n"
    "   x = (x >> 13) ^ x;\n"
    "   return ((x * (x * x * 60493 + 19990303) + 1376312589) & 0x7fffffff)/float(0x7fffffff-1);\n"
    "}\n"

    "void main() {\n"
    "   outvelocity = invelocity;\n"
    "   for(int j = 0;j<3;++j) {\n"
    "       vec3 diff = inposition-center[j];\n"
    "       float dist = length(diff);\n"
    "       float vdot = dot(diff, invelocity);\n"
    "       if(dist<radius[j] && vdot<0.0)\n"
    "           outvelocity -= bounce*diff*vdot/(dist*dist);\n"
    "   }\n"
    "   outvelocity += dt*g;\n"
    "   outposition = inposition + dt*outvelocity;\n"
    "   if(outposition.y < -30.0)\n"
    "   {\n"
    "       outvelocity = vec3(0,0,0);\n"
    "       outposition = 0.5-vec3(hash(3*gl_VertexID+0),hash(3*gl_VertexID+1),hash(3*gl_VertexID+2));\n"
    "       outposition = vec3(0,20,0) + 5.0*outposition;\n"
    "   }\n"
    "}\n";

  // program and shader handles
  GLuint transform_shader_program, transform_vertex_shader;

  // create and compiler vertex shader
  transform_vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  source = transform_vertex_source.c_str();
  length = transform_vertex_source.size();
  glShaderSource( transform_vertex_shader, 1, &source, &length );
  glCompileShader( transform_vertex_shader );
  if ( !check_shader_compile_status( transform_vertex_shader ) ) {
    glfwDestroyWindow( window );
    glfwTerminate();
    return 1;
  }

  // create program
  transform_shader_program = glCreateProgram();

  // attach shaders
  glAttachShader( transform_shader_program, transform_vertex_shader );

  // specify transform feedback output
  const char *varyings[] = { "outposition", "outvelocity" };
  glTransformFeedbackVaryings( transform_shader_program, 2, varyings, GL_INTERLEAVED_ATTRIBS );

  // link the program and check for errors
  glLinkProgram( transform_shader_program );
  check_program_link_status( transform_shader_program );

  GLint center_location = glGetUniformLocation( transform_shader_program, "center" );
  GLint radius_location = glGetUniformLocation( transform_shader_program, "radius" );
  GLint g_location = glGetUniformLocation( transform_shader_program, "g" );
  GLint dt_location = glGetUniformLocation( transform_shader_program, "dt" );
  GLint bounce_location = glGetUniformLocation( transform_shader_program, "bounce" );
  GLint seed_location = glGetUniformLocation( transform_shader_program, "seed" );

  const int particles = 128 * 1024;

  // randomly place particles in a cube
  std::vector<glm::vec3> vertexData( 2 * particles );
  for ( int i = 0; i < particles; ++i ) {
    // initial position
    vertexData[2 * i + 0] = glm::vec3(
                            0.5f - float( std::rand() ) / RAND_MAX,
                            0.5f - float( std::rand() ) / RAND_MAX,
                            0.5f - float( std::rand() ) / RAND_MAX
    );
    vertexData[2 * i + 0] = glm::vec3( 0.0f, 20.0f, 0.0f ) + 5.0f*vertexData[2 * i + 0];

    // initial velocity
    vertexData[2 * i + 1] = glm::vec3( 0, 0, 0 );
  }

  const int buffercount = 2;
  // generate vbos and vaos
  GLuint vao[buffercount], vbo[buffercount];
  glGenVertexArrays( buffercount, vao );
  glGenBuffers( buffercount, vbo );

  for ( int i = 0; i < buffercount; ++i ) {
   
    glBindVertexArray( vao[i] );

    glBindBuffer( GL_ARRAY_BUFFER, vbo[i] );

    // fill with initial data
    glBufferData( GL_ARRAY_BUFFER, sizeof( glm::vec3 )*vertexData.size(), &vertexData[0], GL_STATIC_DRAW );

    // set up generic attrib pointers
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( GLfloat ), (char*)0 + 0 * sizeof( GLfloat ) );
    // set up generic attrib pointers
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( GLfloat ), (char*)0 + 3 * sizeof( GLfloat ) );
  }

  // "unbind" vao
  glBindVertexArray( 0 );

  // we ar blending so no depth testing
  glDisable( GL_DEPTH_TEST );

  // enable blending
  glEnable( GL_BLEND );
  //  and set the blend function to result = 1*source + 1*destination
  glBlendFunc( GL_ONE, GL_ONE );

  // define spheres for the particles to bounce off
  const int spheres = 3;
  glm::vec3 center[spheres];
  float radius[spheres];
  center[0] = glm::vec3( 0, 12, 1 );
  radius[0] = 3;
  center[1] = glm::vec3( -3, 0, 0 );
  radius[1] = 7;
  center[2] = glm::vec3( 5, -10, 0 );
  radius[2] = 12;

  // physical parameters
  float dt = 1.0f / 60.0f;
  glm::vec3 g( 0.0f, -9.81f, 0.0f );
  float bounce = 1.2f; // inelastic: 1.0f, elastic: 2.0f

  BackgroundMusic musicManager;

  musicManager.start();

  int current_buffer = 0;
  while ( !glfwWindowShouldClose( window ) ) {
    glfwPollEvents();
    musicManager.frame();

    // get the time in seconds
    float t = glfwGetTime();

    // use the transform shader program
    glUseProgram( transform_shader_program );

    // set the uniforms
    glUniform3fv( center_location, 3, reinterpret_cast<GLfloat*>( center ) );
    glUniform1fv( radius_location, 3, reinterpret_cast<GLfloat*>( radius ) );
    glUniform3fv( g_location, 1, glm::value_ptr( g ) );
    glUniform1f( dt_location, dt );
    glUniform1f( bounce_location, bounce );
    glUniform1i( seed_location, std::rand() );

    // bind the current vao
    glBindVertexArray( vao[( current_buffer + 1 ) % buffercount] );

    // bind transform feedback target
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo[current_buffer] );

    glEnable( GL_RASTERIZER_DISCARD );

    // perform transform feedback
    glBeginTransformFeedback( GL_POINTS );
    glDrawArrays( GL_POINTS, 0, particles );
    glEndTransformFeedback();

    glDisable( GL_RASTERIZER_DISCARD );

    // clear first
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // use the shader program
    glUseProgram( shader_program );

    // calculate ViewProjection matrix
    glm::mat4 Projection = glm::perspective( 90.0f, 4.0f / 3.0f, 0.1f, 100.f );

    // translate the world/view position
    glm::mat4 View = glm::translate( glm::mat4( 1.0f ), glm::vec3( 0.0f, 0.0f, -30.0f ) );

    // make the camera rotate around the origin
    View = glm::rotate( View, 30.0f, glm::vec3( 1.0f, 0.0f, 0.0f ) );
    View = glm::rotate( View, -22.5f*t, glm::vec3( 0.0f, 1.0f, 0.0f ) );

    // set the uniform
    glUniformMatrix4fv( View_location, 1, GL_FALSE, glm::value_ptr( View ) );
    glUniformMatrix4fv( Projection_location, 1, GL_FALSE, glm::value_ptr( Projection ) );

    // bind the current vao
    glBindVertexArray( vao[current_buffer] );

    // draw
    glDrawArrays( GL_POINTS, 0, particles );

    // check for errors
    GLenum error = glGetError();
    if ( error != GL_NO_ERROR ) {
      std::cerr << error << std::endl;
      break;
    }

    // finally swap buffers
    glfwSwapBuffers( window );

    // advance buffer index
    current_buffer = ( current_buffer + 1 ) % buffercount;
  }

  // delete the created objects

  glDeleteVertexArrays( buffercount, vao );
  glDeleteBuffers( buffercount, vbo );

  glDetachShader( shader_program, vertex_shader );
  glDetachShader( shader_program, geometry_shader );
  glDetachShader( shader_program, fragment_shader );
  glDeleteShader( vertex_shader );
  glDeleteShader( geometry_shader );
  glDeleteShader( fragment_shader );
  glDeleteProgram( shader_program );

  glDetachShader( transform_shader_program, transform_vertex_shader );
  glDeleteShader( transform_vertex_shader );
  glDeleteProgram( transform_shader_program );

  glfwDestroyWindow( window );
  glfwTerminate();
  return 0;
}

