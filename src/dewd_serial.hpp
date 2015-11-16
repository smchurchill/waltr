

class serial_read_session {
public:
  serial_read_session(boost::asio::io_service& io_service,
      std::string location, std::string DATAPATH) :
      port_(io_service), name_(location), timeout_(TIMEOUT), timer_(io_service)
  {
    filename_ = DATAPATH + name_.substr(name_.find_last_of("/\\"));
    port_.open(location);
    name_.resize(11,' ');
    start_ = boost::chrono::steady_clock::now();
    dead_ = start_ + timeout_;

    start_read();
    set_timer();
  }

  /* For now, this function begins the read/write loop with a call to
   * port_.async_read_some with handler handle_read.  It makes a new vector,
   * where the promise to free the memory is honored at the end of handle_read.
   *
   * Later, it will take care of making sure that buffer_, port_, and file_ are
   * sane.  Other specifications are tbd.
   */

private:
  void start_read() {
    std::vector<char>* buffer_ = new std::vector<char>;
    buffer_->assign (BUFFER_LENGTH,0);
    boost::asio::mutable_buffers_1 Buffer_ = boost::asio::buffer(*buffer_);
    boost::asio::async_read(port_,Buffer_,
        boost::bind(&serial_read_session::handle_read, this, _1, _2, buffer_));
  }

  /* This handler first gives more work to the io_service.  The io_service will
   * terminate if it is ever not executing work or handlers, so passing more
   * work to the service at the start of this handler will reduce the possible
   * situations where that could happen.
   *
   * If $length > 0 then there are characters to write, and we write $length
   * characters from buffer_ to file_.  Otherwise, $length=0 and there are no
   * characters ready to write in buffer_.  We increase the count since last
   * write.  If a nonzero number of characters are written then that count is
   * set to 0.
   *
   *
   */
  void handle_read(boost::system::error_code ec, size_t length,
    std::vector<char>* buffer_) {
    /* The first thing we do is give more work to the io_service.  Allocate a
     * new buffer to pass to our next read operation and for our next handler
     * to read from.  That is all done in start().
     */
    this->start_read();

    if (length > 0) {
      file_ = fopen(filename_.c_str(), "a");
      fwrite(buffer_->data(), sizeof(char), length, file_);
      fclose(file_);
    }
    if(0)
    std::cout << "[" << name_ << "]: " << length << " characters written" << std::endl;

    delete buffer_;
  }

  /* This timeout handling is based on the blog post located at:
   *
   * blog.think-async.com/2010/04/timeouts-by-analogy.html
   *
   * The basic idea is that there are two pieces of async work in handled by a
   * serial_read_session: a read and a wait.
   *
   * The async_wait runs the handle_timout handler either when time expires or
   * when the wait operation is canceled.  Nothing will cancel the wait in this
   * program at the moment.
   *
   * The async_read reads characters from the port_ buffer into this session's
   * buffer_ container.  The async_read will exit and run read_handler when
   * either buffer_ is full or timer_ expires and invokes handle_timeout, which
   * cancels all current work operations in progress through port_.
   */
  void set_timer() {
  boost::chrono::time_point<boost::chrono::steady_clock> now_ =
      boost::chrono::steady_clock::now();

  while(dead_< now_)
    dead_ = dead_ + timeout_;
  if(0)
  std::cout << "[" << name_ << "]: deadline set to " << dead_ << '\n';
  timer_.expires_at(dead_);
  timer_.async_wait(
      boost::bind(&serial_read_session::handle_timeout, this, _1));
}

  void handle_timeout(boost::system::error_code ec) {

    if(!ec)
      port_.cancel();

    set_timer();
}




  boost::asio::serial_port port_;
  std::string name_;
  std::string filename_;
  FILE* file_;

  /* Timer specific members */
  boost::chrono::milliseconds timeout_;

  /* November 13, 2015
   *
   * Using:
   *  boost::asio::basic_waitable_timer<boost::chrono::steady_clock>
   *  boost::chrono::time_point<boost::chrono::steady_clock>
   *
   * over:
   *  boost::asio::steady_timer
   *  boost::asio::steady_timer::time_point
   *
   * because the latter does not work.  The typedefs of the asio library should
   * make them act the same, but they don't.  The latter doesn't even compile.
   *
   * May be related to the issue located at:
   *  www.boost.org/doc/libs/1_59_0/doc/html/boost_asio/overview/cpp2011/chrono.html
   *
   *
   */

  boost::asio::basic_waitable_timer<boost::chrono::steady_clock> timer_;
  boost::chrono::time_point<boost::chrono::steady_clock> start_;
  boost::chrono::time_point<boost::chrono::steady_clock> dead_;
};
