# XV6
This project is based on the XV6 operating system by MIT University. 
Each branch of this project is an additional feature or an improvement to the original xv6 kernel.
In the first branch some new features to the console and terminal can be found such as enhanced cursur movement and etc.
The second branch is about how to add some new system calls to the XV6 kernel like a new different version of sleep and a system call that has the same functionality as linux's pstree.
In the third branch we changed the cpu scheduling system which was initially based on round robin. Different scheduling policies such as Lottery and HRRN and Fifo were added.
The fourth branch is about implementation of a memory barrier which is a software level system call used for scheduling different threads or processes, to ensure their desired sequence. Also a simple reentrant lock was implemented to avoid invalid deadlock detection when a process asks for a lock on a resource it already holds.
The last branch is about memory and paging of xv6. As we all know xv6 does not handle demand paging, it loads all the pages of a process when it is forked or executed so none of the pages of the processes remains out of the main memory, so we never experience page fault. To implement the different paging algorithms first we added paging to xv6, in order to do so it is necessary to move some of the pages of a process out of the memory and save it in the disk. After that we added different paging algorithms such as LRU, NFU, Clock, Fifo.

