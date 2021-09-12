# Overview

**paster**: Using libcurl and multiple threads, request a random horizontal strip of picture from server and when recieved all the strips, paste the downloaded png files together.

FLAGS: -t=NUM : number of threads to create. If option not specified, single-threaded implementation of paster will run. -n=NUM: Picture ID. Default value to 1

EXAMPLE: paster -t 6 -n 2



Starter files provided by Bernie Roehl of University of Waterloo
