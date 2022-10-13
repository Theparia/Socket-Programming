# Socket-Programming
The goal of this project is to implement a question and answer system for different courses similar to [Stackoverflow](https://stackoverflow.com/) using socket programming and C language system calls.

In this project, we have a central server that can create rooms for asking questions for a number of fields of study. This server always listens on a certain port so that clients can connect to it. People can connect to the server as a client and give the server the number or name of the field they want to enter and ask questions. Note that the server is a process and each client is a separate process.

The capacity of each room is 3 people. As soon as the number of people in a room reaches the quorum, the server announces a new port to the people of that room so that they can broadcast messages on that port. Each client's connection with the server is of `TCP` type, and after a room is filled, the client's connection with each other will be of `UDP broadcast` type.

Each client has one minute to answer a question. To implement the timer, we use `SIGALRM` signal and `alarm` system call.

Also, in order to synchronize the server and the client without being blocked, we use the `select` system call. This system call can manage communication and I/O without blocking.
