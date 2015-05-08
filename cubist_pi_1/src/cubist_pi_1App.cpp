#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/qtime/QuickTime.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Utilities.h"
#include <sstream>
#include "cinder/Thread.h"
#include "cinder/ConcurrentCircularBuffer.h"

#include <boost/filesystem.hpp>

using namespace boost::filesystem;

using namespace ci;
using namespace ci::app;
using namespace gl;
using namespace std;
using namespace qtime;

#define MAX_CLIPS 5
#define MAX_BUFFER 15

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

    gl::GlslProg            m_shader;
    
    vector<pair<qtime::MovieGlRef, qtime::MovieGlRef>> m_clips;
    ConcurrentCircularBuffer<pair<qtime::MovieGlRef, qtime::MovieGlRef>>* c_clips;
    
    string directory;
    shared_ptr<thread>loader_thread;
    bool should_quit;
    //mutex m_mutex;
};

void cubist_pi_1App::setup()
{

     srand( 133 );
     
     should_quit = false;
     
     directory = "/Users/wolfgag/Desktop/cubistPics/media";
     
     loader_thread = shared_ptr<thread>( new thread( bind( &cubist_pi_1App::loaderThread, this ) ) );
     
     c_clips = new ConcurrentCircularBuffer<pair<qtime::MovieGlRef, qtime::MovieGlRef>>(MAX_BUFFER);

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
     pair<MovieGlRef, MovieGlRef>p;
     if (c_clips->isNotEmpty())
     {
          c_clips->popBack(&p);
          
          m_clips.insert(m_clips.begin(), p);
     }
}

void cubist_pi_1App::draw()
{
    gl::clear( Color( 1.0, 1.0, 1.0 ) );
    gl::enableAlphaBlending();
    gl::enableDepthRead();
    
    
    if (m_shader)
    {
          for (int i = 0; (i < MAX_CLIPS) && (i < m_clips.size()); i++)
          {
               Texture m_frame = m_clips[i].first->getTexture();
               Texture m_frame_m = m_clips[i].second->getTexture();
               m_frame.bind(0);
               m_frame_m.bind(1);
               m_shader.bind();
               m_shader.uniform("t_masked", 0);
               m_shader.uniform("t_mask", 1);
               m_shader.uniform("f_width", (float) m_frame.getSize().x);
               m_shader.uniform("f_height", (float) m_frame.getSize().y);

               Rectf centered_rect = Rectf( m_frame.getBounds() ).getCenteredFit( getWindowBounds(), true );
               centered_rect.scaleCentered(Vec2f(1.0, -1.0));
               gl::drawSolidRect(centered_rect);
             
               m_shader.unbind();
               m_frame.unbind();
               m_frame_m.unbind();
          }
    }
    gl::disableDepthRead();
    gl::disableAlphaBlending();
}

void cubist_pi_1App::shutdown()
{
     should_quit = true;
     c_clips->cancel();
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

void cubist_pi_1App::loaderThread()
{
     ci::ThreadSetup threadSetup;
     while (!should_quit)
     {
          loadMoviesFromDir(directory);
          chrono::seconds delay(120);
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
     
     
     while (c_clips->isNotFull())
     {
          try
          {
               if (! movies.size())
               {
                    return;
               }

               int r =  random() % movies.size();
               
               //float speed = ((random() % 3) + 1) * 0.5;
               
               //maybe MovieGl instead of ref
               qtime::MovieGlRef movie = qtime::MovieGl::create( fs::path(movies[r]) );
               qtime::MovieGlRef mask = qtime::MovieGl::create( fs::path(masks[r]) );
               if (! (movie && mask) )
               {
                    cout << "not" << endl;
                    continue;
               }
               
               if (! (movie->checkPlayable() && mask->checkPlayable()) )
               {
                    cout << "not playable" << endl;
                    continue;
               }
               
               movie->setLoop();
               movie->play();
               movie->setRate(0.15);
               
               mask->setLoop();
               mask->play();
               mask->setRate(0.15);
               
               c_clips->pushFront(pair<MovieGlRef, MovieGlRef>(movie, mask));
          }
          catch(...)
          {
          }
     }

     
}

CINDER_APP_NATIVE( cubist_pi_1App, RendererGl )
