#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "unixatomic.h"
#include "sysincludes.h"

#include "uadeipc.h"

#include <pthread.h>

extern pthread_mutex_t uade_mutex;

volatile char ipc_buffer[2][UADE_MAX_MESSAGE_SIZE];
volatile int ipc_buffer_index[2]={0,0};
volatile int read_write_in_progress[2]={0,0};
extern void uade_dummy_wait();

int atomic_close(int fd)
{
  while (1) {
    if (close(fd) < 0) {
      if (errno == EINTR)
	continue;
      return -1;
    }
    break;
  }
  return 0;
}


int atomic_dup2(int oldfd, int newfd)
{
  while (1) {
    if (dup2(oldfd, newfd) < 0) {
      if (errno == EINTR)
	continue;
      return -1;
    }
    break;
  }
  return newfd;
}



size_t atomic_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	 uint8_t *dest = ptr;
  size_t readmembers = 0;
  size_t ret;

  while (readmembers < nmemb) {
    ret = fread(dest + size * readmembers, size, nmemb - readmembers, stream);
    if (ret == 0)
      break;
    readmembers += ret;
  }

  assert(readmembers <= nmemb);

  return readmembers;
}





void *atomic_read_file(size_t *fs, const char *filename)
{
 FILE *f;
  size_t off;
  void *mem = NULL;
  size_t msize;
  long pos;

  if ((f = fopen(filename, "rb")) == NULL)
    goto error;

  if (fseek(f, 0, SEEK_END))
    goto error;
  pos = ftell(f);
  if (pos < 0)
    goto error;
  if (fseek(f, 0, SEEK_SET))
    goto error;

  *fs = pos;
  msize = (pos > 0) ? pos : 1;

  if ((mem = malloc(msize)) == NULL)
    goto error;

  off = atomic_fread(mem, 1, *fs, f);
  if (off < *fs) {
    fprintf(stderr, "Not able to read the whole file %s\n", filename);
    goto error;
  }

  fclose(f);
  return mem;

 error:
  if (f)
    fclose(f);
  free(mem);
  *fs = 0;
  return NULL;
}



ssize_t atomic_read(int fd, const void *buf, size_t count) {
//	int wait_counter=0;
	if ((fd<0)||(fd>1)) {
		printf("**********************************|:");
		return 0;
	}
	if (count==0) return 0;
	if (count>UADE_MAX_MESSAGE_SIZE) {
		printf("WARNING READING TOO MUCH %d\n",(int)count);
		count=UADE_MAX_MESSAGE_SIZE;
	}
	while (ipc_buffer_index[fd]<count) {
		uade_dummy_wait();  //wait 1ms
		/*wait_counter++;
		if (wait_counter>=5000) {
			printf("break atomic_read : fd%d %d / %d\n",fd,ipc_buffer_index[fd],(int)count);
			break;  //5s
		}*/
	}
	
	
	pthread_mutex_lock(&uade_mutex);
	
	read_write_in_progress[fd]++;
	memcpy((char*)buf,(char*)ipc_buffer[fd],count);
	ipc_buffer_index[fd]-=count;
	if (ipc_buffer_index[fd]) memmove((char*)(ipc_buffer[fd]),(char*)&(ipc_buffer[fd][count]),ipc_buffer_index[fd]);
	
	read_write_in_progress[fd]--;
	pthread_mutex_unlock(&uade_mutex);
	
	return count;
}



ssize_t atomic_write(int fd, const void *buf, size_t count) {
//	int wait_counter=0;
	if ((fd<0)||(fd>1)) {
		printf("**********************************|:");
		return 0;
	}
	fd=fd^1;  //writing socket is the read one on the other side
	if (count==0) return 0;
	if (count>UADE_MAX_MESSAGE_SIZE) {
		printf("WARNING WRITING TOO MUCH %d\n",(int)count);
		count=UADE_MAX_MESSAGE_SIZE;
	}
	while ((ipc_buffer_index[fd]+count)>UADE_MAX_MESSAGE_SIZE) {
		uade_dummy_wait();
		/*wait_counter++;
		if (wait_counter>=5000) {
			printf("break atomic_write : fd%d %d / %d\n",fd,ipc_buffer_index[fd],(int)count);
			break;  //5s
		}*/
	}
	
	pthread_mutex_lock(&uade_mutex);
	
	read_write_in_progress[fd]++;
	memcpy((char*)&(ipc_buffer[fd][ipc_buffer_index[fd]]),(char*)buf,count);
	ipc_buffer_index[fd]+=count;
	read_write_in_progress[fd]--;
	
	pthread_mutex_unlock(&uade_mutex);
	return count;
}
