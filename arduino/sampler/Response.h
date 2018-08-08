// response -------------------------------------------------------------

class Response {
  private:
    Stream* _stream;
    size_t _ssize;
    
    void _print(const char* type, const char* fmt, va_list args) {
      _stream->print(type);
      _stream->print(" ");
      char s[_ssize];
      vsnprintf(s, _ssize, fmt, args);
      _stream->println(s);
    }
    
  public:
    Response(Stream* stream, size_t ssize = 32) {
      _stream = stream;
      _ssize = ssize;
    }
    
    void ok(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      _print("OK", fmt, args);
      va_end(args);
    }
    
    void ok() {
      ok("");
    }
    
    void error(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      _print("ERROR", fmt, args);
      va_end(args);
    }
    
    void error() {
      error("");
    }
    
    void print(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      char s[_ssize];
      vsnprintf(s, _ssize, fmt, args);
      _stream->print(s);
      va_end(args);
    }
    
    void println(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      char s[_ssize];
      vsnprintf(s, _ssize, fmt, args);
      _stream->println(s);
      va_end(args);
    }
    
    void println() {
      println("");
    }
    
    void debug(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      char s[_ssize];
      vsnprintf(s, _ssize, fmt, args);
      _stream->print("# ");
      _stream->println(s);
      va_end(args);
    }
};

Response response = Response(&Serial);
