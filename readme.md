# Simple Web Server

Simple web server written in C++,

## Configuration

The configuration file is `config.txt` and located at the same folder of this app.

	/home/sonny/Documents/PAT/
	3900

1. the first line indicate where is the root of our webserver folder (include slash at the end)
2. the second line is the port number you want to listen

## Compile & Run

	make
	make run

## Custom Port

if you are cli-fanboy, you can use the first command argument for custom port

	make run port=3700

## Testing & Benchmarking

You may try the web server with the default configuration from the browser, just open

	http://localhost:3900/

You will be given hello world page. You can test the performance of the web server by opening this in your browser

	http://localhost:3900/test.html

If you wanna test the concurrency, you may use apache bench. This is the step to install apache bench

- Install apache2

        sudo apt-get update
        sudo apt-get install apache2

- install apache2 utils

        sudo apt-get install apache2-utils

- test with concurrency and number of users

        ab -n1000 -c100 http://localhost:3900/

