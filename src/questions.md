Timeouts:
    - Correct timeout value for the Send() loop
    - Correct timeout value for SendAck() loop
    - Correct timeout value for removing an ack from the set of acks to send
    - Make them dynamic?

Logging:
    - When should we print that we sent a message? Only one we know the receiver has received it? Or right after the sending of the first packet?

Threads:
    - Should all memory be free'd even when stoping with Ctrl + C?
    - Should I use thread::detach or thread::join (perhaps related to memory leaks)

TODO:
    - Implement failure detector