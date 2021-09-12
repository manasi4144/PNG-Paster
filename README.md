# Overview

**paster**: Using libcurl and multiple threads, request a random horizontal strip of picture from server and when recieved all the strips, paste the downloaded png files together.

FLAGS: -t=NUM : number of threads to create. If option not specified, single-threaded implementation of paster will run. -n=NUM: Picture ID. Default value to 1

EXAMPLE: paster -t 6 -n 2

paster2: Uses producer threads make requests to the lab web server until they fetch all 50 distinct image segments. Each consumer thread reads image segments out of the buffer, one at a time, and then sleeps for X milliseconds specified by the user in the command line. Then the consumer will process the received data and paste the segments together.

FLAGS: ./paster2 B P C X N B: buffer size P: Number of producers C: Number of Consumers X : Milliseconds consumer threads sleeps N : Picture ID. Default value to 1

Starter files provided by Bernie Roehl of University of Waterloo
