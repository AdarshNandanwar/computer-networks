## doubts
1. Port and IP address input (STDIN or parameter)
2. reverse each character or word by word
3. should client program terminate after it gets a response from the server?
4. Assume line size to be 1024 bytes max
5. take port and ip as command line arguement (not stdin)
# ToDo
- Fix string input, its currently taking only first word.
- In wireshark, add all the required columns
- Show all the required calculations if the required answer is not shown in some column. eg. the size of the TCP segment.
## testing
1. exit the client when client is typing using ctrl+c and see if new client rejoins, if the server hangs
2. exit the client when server is typing using ctrl+c and see if new client rejoins, if the server hangs
