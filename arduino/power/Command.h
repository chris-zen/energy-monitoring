// command -----------------------------------------------------------

#define CMD_END1 ';'
#define CMD_END2 '\n'

#define MAX_CMD_SIZE 24

class Command {
  private:
    Stream* _stream;
    char _cmd[MAX_CMD_SIZE];
    unsigned int _cmd_len;
    
    boolean is_end(char ch) {
      return ch == CMD_END1 || ch == CMD_END2;
    }
  
  public:
    Command(Stream* stream) {
      _stream = stream;
      _cmd[0] = 0;
      _cmd_len = 0;
    }
    
    const char* read() {
      boolean done = false;
      _cmd_len = 0;
      while (!done) {
        if (_stream->available() > 0) {
          // Check for command overflow
          if (_cmd_len >= MAX_CMD_SIZE) {
            // Discard all the command
            while (!is_end(_stream->read()));
            _cmd_len = 0;
            _cmd[0] = 0;
            while (_stream->available() == 0);
          }
          
          char ch = _stream->read();
          if (is_end(ch)) {
            ch = 0;
            done = true;
          }
          _cmd[_cmd_len++] = ch;
        }
      }
      return _cmd;
    }
};

