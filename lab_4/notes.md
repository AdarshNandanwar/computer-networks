# FIFO pipe / named pipe
- It have to be open on both ends for it to work.
- If one process `open()` in `O_WRONLY`, then it will block untill some other process opens the file for reading. e.g. `cat`
- Same if a process `open()` in `O_RDONLY`, then it will block untill some other process opens the file for writing.
- Same if a process `open()` in `O_RDWR`, then it won't block as both the read and write ends are open by the same process.