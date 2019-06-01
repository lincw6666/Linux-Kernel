Project 04
===

# Introduction

"romfs" is a read-only file system. In this project, we are going to trace how it works, and we'll modify some codes to complete three tasks:
- Hide the files and directories with the same name as "hided_file_name"
- Encrypt the content of the file with the same name as "encrypted_file_name"
- Set the executable bit of the file with the same name as "exec_file_name"

Reference: [romfs](https://github.com/torvalds/linux/blob/master/Documentation/filesystems/romfs.txt)

----

# Setup

- Create a directory with some files and directories.
- Build genromfs
	- Download from [here](http://romfs.sourceforge.net/), in the development section.
	- Unzip it and run `make`.
	- Create a romfs image:
		```sh
		./genromfs -d your_target_dir -f your_romfs_img -V "your romfs device name"
		```

----

# Usage

- Create the kernel module:
	```sh
	make clean; make
	```
- Insert the kernel module
	- For task 01:
		```sh
		sudo insmod romfs.ko hided_file_name=XXX
		```
	- For task 02:
		```sh
		sudo insmod romfs.ko encrypted_file_name=XXX
		```
	- For task 03:
		```sh
		sudo insmod romfs.ko exec_file_name=XXX
		```
- Mount the romfs image under a directory:
	```sh
	sudo mount -o loop your_romfs_img your_mount_ponit
	```
- After you finish the project, unmount the romfs image and remove the kernel module:
	```sh
	sudo umount your_romfs_img
	sudo rmmod romfs
	```
- Example:
	```sh
	# Under "romfs/".
	make clean; make
	# For task 01.
	sudo insmod romfs.ko hided_file_name=aa

	# Under the directory where your romfs image at.
	cd ../
	# Mount the romfs image.
	sudo mount -o loop test.img t

	# Task 01.
	# Now you will not see "aa" under "t/".
	find t

	# Finish the expirment.
	sudo umount test.img
	sudo rmmod romfs
	```

----

# Prerequest

Let the kernel module gets three inputs:
- **hided_file_name**
- **encrypted_file_name**
- **exec_file_name**.

```c
// Static variables.
static char *hided_file_name = "";
static char *encrypted_file_name = "";
static char *exec_file_name = "";

module_param(hided_file_name,		charp,	S_IRUSR | S_IWUSR);
module_param(encrypted_file_name,	charp,	S_IRUSR | S_IWUSR);
module_param(exec_file_name,		charp,	S_IRUSR | S_IWUSR);
```


----

# Task 1

**TODO**: Hide the files and directories with the same name as "hided_file_name".

## Analysis

- See what happens when we enter `find your_romfs_dir`.
    ![](https://i.imgur.com/8p7xP1O.png)
- ***romfs_readdir*** iterates all files and directories under the current directory.
- ***romfs_lookup*** looks into the dirctory. It'll create an inode for the directory. In this case, *romfs_lookup* looks into *"t/fo/"*.
- The function we need to modify is ***romfs_readdir***.

## Code

- Focus on the ***romfs_readdir*** function.
- ***fsname*** is the file name. The following codes show how to get the file name.
    ![](https://i.imgur.com/T0ztnYo.png)
- ***dir_emit*** will push the file list into some data buffer. The content of the data buffer will be shown on the screen. Therefore, we don't want the file with the same name as "hided_file_name" to enter this function.
    ![](https://i.imgur.com/SuolYlU.png)
- We add some codes **after getting the file name and before entering dir_emit**.
    ```c
    j = romfs_dev_strnlen(i->i_sb, offset + ROMFH_SIZE,
				      sizeof(fsname) - 1);
	if (j < 0)
		goto out;

	ret = romfs_dev_read(i->i_sb, offset + ROMFH_SIZE, fsname, j);
	if (ret < 0)
		goto out;
	fsname[j] = '\0';

	ino = offset;
	nextfh = be32_to_cpu(ri.next);
	/* Skip directories or files which name are "hided_file_name". */
	if (strcmp(hided_file_name, fsname) == 0)
			goto skip_name;
	if ((nextfh & ROMFH_TYPE) == ROMFH_HRD)
		ino = be32_to_cpu(ri.spec);
	if (!dir_emit(ctx, fsname, j, ino,
		    romfs_dtype_table[nextfh & ROMFH_TYPE]))
		goto out;
		
	skip_name:
	    offset = nextfh & ROMFH_MASK;
    ```

----

# Task 2

**TODO**: Encrypt the content of the file with the same name as "encrypted_file_name".

## Analysis

- See what happen when we enter `cat your_target_file`.
    In this case, I entered `cat t/bb`.
    ![](https://i.imgur.com/TOITCkv.png)
- ***romfs_lookup*** creates an inode for *"t/bb"*, which includes all needed informations, including meta size and data offset.
- ***romfs_readpage*** gets the data in the target file. It reads a *PAGE_SIZE* (usually 4KB) of data. If the data size is smaller than a page, it pads zero to the remain part.

The function we need to modify is ***romfs_readpage***.
- The only information we have is the **inode** of the file.
- Use **meta size** and **data offset** to get the position of the file in the romfs image.

Let's look at how ***romfs_iget*** stores the meta size and data offset in an inode:
![](https://i.imgur.com/iyw9G6x.png)
- **nlen**: The length of the file name.
- **ROMFH_SIZE** = **16**: The size of the file header.
- **ROMFH_PAD** = **15**: The file header must begins on a **16 byte boundary**. Therefore, we add it to the meta data size and mask by **ROMFH_MASK** (= **~ROMFH_PAD**).

By these information, we can get the position, where the file name starts, by
```c
pos = ROMFS_I(inode)->i_dataoffset - ROMFS_I(inode)->i_metasize + offset + ROMFH_SIZE
```
in ***romfs_readpage***. (*offset*: the page offset)

## Code

- Focus on the ***romfs_readpage*** function.
- The data in the file is written into the `buf` variable.
    ![](https://i.imgur.com/CLhULBF.png)

    We need to avoid this if the file name is the same as "encrypted_file_name". Therefore, we add another if statement in front of it.
    ```c
    /*
	 * Get the file name. If it is the same as "encrypted_file_name",
	 * write "ccccccccccccccc" to @buf.
	 */
	pos = ROMFS_I(inode)->i_dataoffset - ROMFS_I(inode)->i_metasize + offset + ROMFH_SIZE;
	if (compare_file_name(inode->i_sb, pos, encrypted_file_name)) {
		// Write "ccccccccccccccc" to @buf.
		strcpy(buf, "ccccccccccccccc");
		// We need to update @fillsize or the content from @buf+fillsize
		// to the end will be 0.
		fillsize = 15;	// Length of "cccccccccccccc".
	}
	else if (offset < size) {
		size -= offset;
		fillsize = size > PAGE_SIZE ? PAGE_SIZE : size;

		pos = ROMFS_I(inode)->i_dataoffset + offset;

		ret = romfs_dev_read(inode->i_sb, pos, buf, fillsize);
		if (ret < 0) {
			SetPageError(page);
			fillsize = 0;
			ret = -EIO;
		}
	}
    ```
- We **MUST** update `fillsize` or the content in `buf` will be set to 0 in the following if statement:
    ![](https://i.imgur.com/miLtRsm.png)
- Here is the `compare_file_name` function:
    ```c
    static bool compare_file_name(struct super_block *sb, unsigned long pos, char *name) {
        int j, ret;
        char fsname[ROMFS_MAXFN];

        j = romfs_dev_strnlen(sb, pos, sizeof(fsname)-1);
        if (j < 0)
            return false;

        ret = romfs_dev_read(sb, pos, fsname, j);
        if (ret < 0)
            return false;
        fsname[j] = '\0';

        if (strcmp(name, fsname) == 0)
            return true;
        return false;
    }
    ```

## Remind: Encrypt empty file

If you try to encrypt the empty file by the method above, you'll find that it doesn't work.
Why? Let's look in to the **file operations**.
In ***romfs_iget***, we can see that it creates an inode and assigns file operations `romfs_ro_fops` to it. Here is the `romfs_ro_fops`:
```c
const struct file_operations romfs_ro_fops = {
	.llseek			= generic_file_llseek,
	.read_iter		= generic_file_read_iter,
	.splice_read		= generic_file_splice_read,
	.mmap			= romfs_mmap,
	.get_unmapped_area	= romfs_get_unmapped_area,
	.mmap_capabilities	= romfs_mmap_capabilities,
};
```
We can see that it calls `read_iter` when we read the file. Let's look into ***generic_file_read_iter**. Here's the answer why we can't encrypt an empty file:
```c
/*
 * Btrfs can have a short DIO read if we encounter
 * compressed extents, so if there was an error, or if
 * we've already read everything we wanted to, or if
 * there was a short read because we hit EOF, go ahead
 * and return.  Otherwise fallthrough to buffered io for
 * the rest of the read.  Buffered reads will not work for
 * DAX files, so don't bother trying.
 */
if (retval < 0 || !count || iocb->ki_pos >= size ||
    IS_DAX(inode))
    goto out;
```
Here's the key comment:
> if there was a short read because we hit EOF

If the file is empty, `iocb->ki_pos >= size` is true then it won't show anything on the screen.
We can use a trick that we set the size to non-zero for the corresponding inode. But I thought that it was tricky, I didn't do so. Just leave it empty on the screen.

----

# Task 3

**TODO**: Set the executable bit of the file with the same name as "exec_file_name".

## Analysis

- See what happen when we enter `ls -l your_romfs_dir`.
    In this case, we can see that it creates inodes for "aa", "bb", "ft" and the directories.
    ![](https://i.imgur.com/UALYGVd.png)
- Let's look into ***romfs_iget***. It creates the inode and stores the mode of the file into the inode. The following codes show where it sets the executable bit of the file.
    ![](https://i.imgur.com/jua8Wvk.png)
    All we need to do is to add an extra condition, which compares the file name with "exec_file_name", in the if statement.
    
## Code

```c
case ROMFH_REG:
    i->i_fop = &romfs_ro_fops;
    i->i_data.a_ops = &romfs_aops;
    if (nextfh & ROMFH_EXEC
        || compare_file_name(sb, pos + ROMFH_SIZE, exec_file_name))
        mode |= S_IXUGO;
    break;
```



