echo -en "GETFILE GET /courses/ud923/filecorpus/1kb-sample-file-1.html \r\n\r\n" | netcat localhost 6200 | hexdump -C
