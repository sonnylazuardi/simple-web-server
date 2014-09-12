/* Simple Web Server */

/* Sonny Lazuardi Hermawan - 13511029 */
/* Muhammad Harits SAHE - 13511046 */

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <stdlib.h>
#include <ev++.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <resolv.h>
#include <errno.h>
#include <list>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

/* Global variable */
string file_content;
bool file_status;

/* Variable konfigurasi (default) */
string file_conf = "config.txt";
int conf_port = 8200;
string conf_file_dir = "/home/sonny/Documents/PAT/";

/* method untuk membaca file, konten dari file akan masuk di global var file_content */
void read_file (const string filename) {
  string line;
  ifstream myfile (filename.c_str());
  file_content = "";
  file_status = false;
  if (myfile.is_open())
  {
    file_status = true;
    while ( getline (myfile,line) )
    {
      file_content += line;
      file_content += "\n";
    }
    myfile.close();
  }
  else cout << "Unable to open file : " << filename << endl;
}

/* string tokenizer, pisahkan dengan karakter tertentu kedalam vector of string */
vector<string> split(const char *str, char c = ' ')
{
    vector<string> result;

    do
    {
    const char *begin = str;

    while(*str != c && *str)
        str++;

    result.push_back(string(begin, str));
    } while (0 != *str++);

    return result;
}

void read_conf() {
    read_file(file_conf);
    vector<string> conf_line = split(file_content.c_str(), '\n');
    conf_file_dir = conf_line[0];
    conf_port = atoi(conf_line[1].c_str());
}
 
/* Kelas Buffer, untuk buffer output dan ditulis secara asinkronus */
struct Buffer {
    char       *data;
    ssize_t len;
    ssize_t pos;
 
    Buffer(const char *bytes, ssize_t nbytes) {
        pos = 0;
        len = nbytes;
        data = new char[nbytes];
        memcpy(data, bytes, nbytes);
    }
 
    virtual ~Buffer() {
        delete [] data;
    }
 
    char *dpos() {
        return data + pos;
    }
 
    ssize_t nbytes() {
        return len - pos;
    }
};
 
/* Sebuah single instance untuk non-blocking echo handler */
class EchoInstance {
private:
    ev::io       io;
    int          sfd;
 
    /* Buffer yang dipending untuk ditulis */
    std::list<Buffer*>     write_queue;
 
    /* Callback umum */
    void callback(ev::io &watcher, int revents) {
        if (EV_ERROR & revents) {
            perror("got invalid event");
            return;
        }
 
        if (revents & EV_READ)
            read_cb(watcher);
 
        if (revents & EV_WRITE)
            write_cb(watcher);
 
        if (write_queue.empty()) {
            io.set(ev::READ);
        } else {
            io.set(ev::READ|ev::WRITE);
        }
    }
 
    /* Socket dapat ditulis */
    void write_cb(ev::io &watcher) {
        if (write_queue.empty()) {
            io.set(ev::READ);
            return;
        }
 
        Buffer* buffer = write_queue.front();
 
        ssize_t written = write(watcher.fd, buffer->dpos(), buffer->nbytes());
        if (written < 0) {
            perror("read error");
            return;
        }
 
        buffer->pos += written;
        if (buffer->nbytes() == 0) {
            write_queue.pop_front();
            delete buffer;
        }

        /* Tutup dan bebaskan watcher saat selesai mengirim response (stateless) */
        io.stop();
        close(sfd);
    }
 
    /* Menerima pesan dari socket client */
    void read_cb(ev::io &watcher) {
        char       buffer[1024];
 
        ssize_t   nread = recv(watcher.fd, buffer, sizeof(buffer), 0);

        if (nread < 0) {
            perror("read error");
            return;
        }
 
        if (nread == 0) {
            delete this;
        } else {
            vector<string> pisah = split(buffer, '\n');
            vector<string> pisah2 = split(pisah[0].c_str(), ' ');
            string current_url = pisah2[1];
            current_url.erase(current_url.begin());

            if (current_url == "") {
                current_url = "index.html";
            }

            string file_path = conf_file_dir + current_url;
            read_file(file_path);
            string content = file_content;
            string header = "";

            /* Mengirim pesan berdasarkan status file */
            if (file_status) {
                header += "HTTP/1.1 200 OK\r\n";
                header += "Content-Type: text/html\r\n";
                header += "Accept-Ranges: bytes\r\n";
                header += "Server: Apache/2.4.7 (Ubuntu)\r\n";
                header += "Vary: Accept-Encoding\r\n";
                header += "Content-Type: text/html\r\n";
                header += "Content-Length: ";
                stringstream ss;
                ss << content.length()+1;
                header += ss.str();
                header += "\r\n";
                header += "\r\n";
                header += "\r\n";
            } else {
                content = "<h1>Not Found</h1>";

                header += "HTTP/1.1 404 Not Found\r\n";
                header += "Content-Type: text/html\r\n";
                header += "Content-Length: ";
                stringstream ss;
                ss << content.length()+1;
                header += ss.str();
                header += "\r\n";
                header += "\r\n";
                header += "\r\n";    
            }

            string balik = header + content;
            write_queue.push_back(new Buffer(balik.c_str(), balik.length()));
        }
    }
 
    /* Hancurkan dan tutup */
    virtual ~EchoInstance() {
        /* Tutup dan bebaskan watcher saat socket ditutup */
        io.stop();
        close(sfd);
    }
 
public:
    EchoInstance(int s) : sfd(s) {
        fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
        printf("Client terkoneksi\n");
        io.set<EchoInstance, &EchoInstance::callback>(this);
        io.start(s, ev::READ);
    }
};
 
class EchoServer {
private:
    ev::io       io;
    ev::sig     sio;
    int         s;
 
public:
 
    void io_accept(ev::io &watcher, int revents) {
        if (EV_ERROR & revents) {
            perror("event tidak valid");
            return;
        }
 
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
 
        int client_sd = accept(watcher.fd, (struct sockaddr *)&client_addr, &client_len);
 
        if (client_sd < 0) {
            perror("mendapatkan error");
            return;
        }
 
        EchoInstance *client = new EchoInstance(client_sd);
    }
 
    static void signal_cb(ev::sig &signal, int revents) {
        signal.loop.break_loop();
    }
 
    EchoServer(int port) {
        printf("Mendengarkan pada port %d\n", port);
 
        struct sockaddr_in addr;
 
        s = socket(PF_INET, SOCK_STREAM, 0);
 
        addr.sin_family = AF_INET;
        addr.sin_port     = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
 
        if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            perror("bind");
        }
 
        fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
 
        listen(s, 5);
 
        io.set<EchoServer, &EchoServer::io_accept>(this);
        io.start(s, ev::READ);
 
        sio.set<&EchoServer::signal_cb>();
        sio.start(SIGINT);
    }
 
    virtual ~EchoServer() {
        shutdown(s, SHUT_RDWR);
        close(s);
    }
};
 
/* Program Utama */
int main(int argc, char **argv)
{
    read_conf();

    if (argc > 1)
        conf_port = atoi(argv[1]);
 
    ev::default_loop       loop;
    EchoServer           echo(conf_port);
    
    /* Menjalankan loop server */
    loop.run(0);
 
    return 0;
}