#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/qtime/QuickTime.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Utilities.h"
#include <sstream>
#include <pthread.h>

#include <boost/filesystem.hpp>

using namespace boost::filesystem;

using namespace ci;
using namespace ci::app;
using namespace gl;
using namespace std;

#define MAX_CLIPS 5

/*
CIRCULAR BUFFER LOADS MOVIES, THEN MAIN THREAD COPIES THEM INTO VECTOR
ALSO VERSION-CONTROL THIS FIRST
*/


struct recursive_directory_range
{
    typedef recursive_directory_iterator iterator;
    recursive_directory_range(path p) : p_(p) {}

    iterator begin() { return recursive_directory_iterator(p_); }
    iterator end() { return recursive_directory_iterator(); }

    path p_;
};

class cubist_pi_1App : public AppNative {
  public:
	void setup();
	//void mouseDown( MouseEvent event );
	void update();
	void draw();
    void shutdown();
    
    void loadMovieFile( const fs::path &path_, qtime::MovieGlRef &movie_ );
    void loadMoviesFromDir(string dir_);
    void loaderThread();
    string removeExtension(string path);
    
    gl::GlslProg            m_shader;
    
    vector<fs::path> m_paths;
    vector<pair<qtime::MovieGlRef, qtime::MovieGlRef>> m_clips;
    vector<qtime::MovieGlRef> m_movies;
    vector<qtime::MovieGlRef> m_masks;
    
    string directory;
    shared_ptr<thread>loader_thread;
    bool should_quit;
    mutex m_mutex;
};

void cubist_pi_1App::setup()
{

     srand( 133 );
     
     should_quit = false;
     
     directory = "/Users/wolfgag/Desktop/cubistPics/media";
     
     loader_thread = shared_ptr<thread>( new thread( bind( &cubist_pi_1App::loaderThread, this ) ) );

    try {
		m_shader = gl::GlslProg( loadAsset("glsl/vert_pt.glsl"), loadAsset( "glsl/frag_mask.glsl" ) );
	}
	catch( gl::GlslProgCompileExc &exc ) {
		std::cout << "Shader compile error: " << std::endl;
		std::cout << exc.what();
	}
    
}

//void cubist_pi_1App::mouseDown( MouseEvent event )
//{
//}


void cubist_pi_1App::update()
{
//     if ()
//     {
//          //IN CINDER YOU CANNOT INITIALIZE A TEXTURE WITH ANOTHER TEXTURE FOOL
//          m_frame = m_movies.back()->getTexture();
//          m_frame_m = m_masks.back()->getTexture();
//     }
}

void cubist_pi_1App::draw()
{
    gl::clear( Color( 1.0, 1.0, 1.0 ) );
    gl::enableAlphaBlending();
    gl::enableDepthRead();
    
    
    if (m_shader)
    {
          m_mutex.lock();
          //cout << "3:" << m_movies.size() << ", " << m_masks.size() << endl;
          for (int i = 0; (i < m_movies.size()) && (i < m_masks.size()); i++)
          {
               Texture m_frame = m_movies[i]->getTexture();
               Texture m_frame_m = m_masks[i]->getTexture();
               m_frame.bind(0);
               m_frame_m.bind(1);
               m_shader.bind();
               m_shader.uniform("t_masked", 0);
               m_shader.uniform("t_mask", 1);
               m_shader.uniform("f_width", (float) m_frame.getSize().x);
               m_shader.uniform("f_height", (float) m_frame.getSize().y);

               Rectf centered_rect = Rectf( m_frame.getBounds() ).getCenteredFit( getWindowBounds(), true );
               gl::drawSolidRect(centered_rect);
             
               m_shader.unbind();
               m_frame.unbind();
               m_frame_m.unbind();
               //cout << "4:" << m_movies.size() << ", " << m_masks.size() << endl;
          }
          m_mutex.unlock();
          
    }
    

    gl::disableDepthRead();
    gl::disableAlphaBlending();
}

void cubist_pi_1App::shutdown()
{
     should_quit = true;
     loader_thread->join();
}

void cubist_pi_1App::loadMovieFile( const fs::path &path_, qtime::MovieGlRef &movie_ )
{
	try {
		// load up the movie, set it to loop, and begin playing
		movie_ = qtime::MovieGl::create( path_ );
		movie_->setLoop();
		movie_->play();
	}
	catch( ... ) {
		console() << "Unable to load the movie." << std::endl;
		movie_->reset();
	}
}

string removeExtension(string path_)
{
     int lastindex = path_.find_last_of(".");
     return path_.substr(0, lastindex);
}

void cubist_pi_1App::loaderThread()
{
     while (!should_quit)
     {
          loadMoviesFromDir(directory);
          chrono::seconds delay(20);
          this_thread::sleep_for(delay);
     }
}

void cubist_pi_1App::loadMoviesFromDir(string dir_)
{
     vector<string>movies;
     vector<string>masks;
     for (auto it : recursive_directory_range(dir_))
     {
          string s = it.path().string();
          if (s.find("cam") != -1)
          {
               if (s.at(s.find_last_of(".") - 1) == 'm')
               {
                    masks.push_back(s);
               }
               else
               {
                    movies.push_back(s);
               }
          }
     }
     
     m_mutex.lock();
     
     m_movies.clear();
     m_masks.clear();
     for (int i = 0; i < MAX_CLIPS; i++)
     {
          try
          {
               int r = random() % movies.size();
               //cout << movies.size() << ", " << masks.size() << endl;
               
//               float speed = ((random() % 3) + 1) * 0.5;
               
               //maybe MovieGl instead of ref
               qtime::MovieGlRef movie = qtime::MovieGl::create( fs::path(movies[r]) );
               movie->setLoop();
               movie->play();
               movie->setRate(0.15);
               m_movies.push_back(movie);
               
               //cout << "1:" << m_movies.size() << ", " << m_masks.size() << endl;
               
               qtime::MovieGlRef mask = qtime::MovieGl::create( fs::path(masks[r]) );
               mask->setLoop();
               mask->play();
               mask->setRate(0.15);
               m_masks.push_back(mask);
               
               //cout << "2:" << m_movies.size() << ", " << m_masks.size() << endl;
               
               //m_clips.push_back(pair<qtime::MovieGlRef, qtime::MovieGlRef>(movie, mask));
          }
          catch(...)
          {
               m_movies.clear();
               m_masks.clear();
          }
     }
     
    m_mutex.unlock();
     
}

CINDER_APP_NATIVE( cubist_pi_1App, RendererGl )
