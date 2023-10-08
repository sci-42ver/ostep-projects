#include <stdio.h>
#include "udp.h"
#include "../mfs.h"

void check_rc(int rc) {
  if (rc == -1) {
    exit(EXIT_FAILURE);
  }
}

void error_log() { fprintf(stderr, "The following one should fail\n"); }

// client code
int main(int argc, char *argv[]) {
  /*
  1. send to localhost:10000
  2. function as part of "MFS_Init".
  */
  int rc = MFS_Init("localhost", 10000);
  assert(rc != -1);

  rc = MFS_Lookup(ROOT_INUM, ".");
  check_rc(rc);

  MFS_Stat_t file_stat;
  rc = MFS_Stat(ROOT_INUM, &file_stat);
  check_rc(rc);
  /*
  0 -> checkpoint
  1 -> root data
  2 -> root inode and related imap
  */
  uint end_block = 2;

  uint inum = ROOT_INUM;
  rc = MFS_Creat(ROOT_INUM, REGULAR_FILE, "foo");
  inum++;
  /*
  3 -> new root data
  4 -> file inode, root inode and related imap
  */
  end_block += 2;
  check_rc(rc);

  char data_to_send[BSIZE] = {0};
  sprintf(data_to_send, "test first\n");
#ifdef TEST_FAIL_WRITE
  error_log();
  rc = MFS_Write(inum, data_to_send, 10); // should fail
#endif
  rc = MFS_Write(inum, data_to_send, end_block + 1);
  /*
  5-> file data
  6 -> file inode and related imap
  */
  end_block += 2;
  check_rc(rc);

  memset(data_to_send, 0, BSIZE);
#ifdef TEST_FAIL_WRITE
  /*
  should fail because I write to the consecutive block after Checkpoint->log_end
  */
  error_log();
  rc = MFS_Read(inum, data_to_send, 10);
#endif
  rc = MFS_Read(inum, data_to_send, end_block - 1);
  check_rc(rc);

  rc = MFS_Creat(ROOT_INUM, DIRECTORY, "bar");
  inum++;
  /*
  7-> new dir data
  8 -> root data
  9 -> new dir inode, root inode and related imap
  */
  end_block += 3;
  check_rc(rc);

  rc = MFS_Creat(inum, REGULAR_FILE, "foo");
  inum++;
  /*
  10-> new root data
  11(0xb)-> file inode, pdir (parent dir) inode and related imap
  */
  end_block += 2;
  check_rc(rc);

  memset(data_to_send, 0, BSIZE);
  sprintf(data_to_send, "test second\n");
  rc = MFS_Write(inum, data_to_send, end_block + 1);
  /*
  12-> file data
  13(0xd)-> file inode and related imap
  */
  end_block += 2;
  check_rc(rc);

  memset(data_to_send, 0, BSIZE);
  rc = MFS_Read(inum, data_to_send, end_block - 1);
  check_rc(rc);

  rc = MFS_Lookup(inum - 1, "foo");
  check_rc(rc);

  rc = MFS_Stat(inum, &file_stat); // /bar/foo
  check_rc(rc);

  error_log();
  rc = MFS_Unlink(ROOT_INUM, "bar");

  /*
  -> new pdir data block
  -> pdir inode and imaps related with pdir and the file
  */
  rc = MFS_Unlink(ROOT_INUM, "foo");
  end_block += 2;
  check_rc(rc);

  error_log();
  rc = MFS_Lookup(ROOT_INUM, "foo");

  /*
  1. foo bar ...
  https://en.wikipedia.org/wiki/Metasyntactic_variable#General_usage
  2. this should use the invalid inum of foo -> 1.
  */
  rc = MFS_Creat(ROOT_INUM, REGULAR_FILE, "baz");
  /*
  14-> new root data
  15-> file inode, root inode and related imap
  */
  end_block += 2;
  check_rc(rc);

  rc = MFS_Lookup(ROOT_INUM, "baz");
  check_rc(rc);

  rc = MFS_Unlink(inum - 1, "foo");
  check_rc(rc);

  rc = MFS_Unlink(ROOT_INUM, "bar");
  check_rc(rc);

  return 0;
}
