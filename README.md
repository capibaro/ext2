# ext2

A naive implementation of [ext2](https://wiki.osdev.org/Ext2) file system. We use virtual disk interface to simulate a 4MB disk. Each read and write needs to read and write a full 512B disk block.

## Build and run

### build

`gcc disk.c util.c sh.c ext2.c -o ext2`

### run

`.\ext2`

## Usage

### touch

`touch <file>`

### mkdir

`mkdir <dir>`

### cp

`cp <src> <dst>`

### ls

`ls [dir]`

### shutdown

`shutdown`

#### Example

```
naive ext2 is booting...
no existing file system found
a new file system will be built
>>>touch a b c
>>>ls
a b c
>>>mkdir d e f
>>>ls
a b c d e f
>>>cp a d/g
>>>ls d
g
>>>mkdir e/h
>>>touch e/h/i e/h/j
>>>ls e/h
i j
>>>cp e/h/j k
>>>ls
a b c d e f k
>>>shutdown
```

```
naive ext2 is booting...
existing file system found
format disk?(y/n)n
>>>ls
a b c d e f k 
>>>shutdown
```