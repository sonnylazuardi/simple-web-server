# Simple Web Server

Simple web server written in C++,

## Configuration

The configuration file is config.txt and located at the same folder of this app.

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