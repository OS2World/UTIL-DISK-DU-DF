/*************************************************
 * DF (*nix like) for  OS/2 v2.x & DOS
 * Copyright (c) 1993,1994 Timo Kokkonen.
 *
 * compiler: Borland C++ for OS/2 or DOS
 */

#include <stdio.h>
#include <string.h>
#include <dir.h>
#include <dos.h>

#ifdef __OS2__
 #define INCL_DOSFILEMGR
 #define INCL_DOSMISC
 #include <os2.h>
 #define VERSION_BITS "32"
#else
 #define VERSION_BITS "16"
#endif

#define RELEASE_DATE "01-Jan-94"
#define VERSION_NO   "1.11"


/**********************************************************************/

int getDiskSize(unsigned char drive,unsigned long *size,
				    unsigned long *free,
				    unsigned long *clustersize)
{
  struct diskfree_t   	data;

  if(_dos_getdiskfree(drive,&data)==0) {
    *size=(unsigned long)data.total_clusters*data.bytes_per_sector*
	  data.sectors_per_cluster;
    *free=(unsigned long)data.avail_clusters*data.bytes_per_sector*
	  data.sectors_per_cluster;
    *clustersize=(unsigned long)data.bytes_per_sector*
		 data.sectors_per_cluster;
    return 1;
  }
  *size=*free=*clustersize=0;
  return 0;
}

void getDiskLabel(unsigned char drive, char *label)
{
 #ifdef __OS2__
  struct {
    ULONG volID;
    BYTE  labellen;
    char  label[40];
  } buf;
  if (DosQueryFSInfo(drive,2,&buf,sizeof(buf))!=0) strcpy(label,"");
    else strncpy(label,buf.label,40);
 #else
  struct ffblk ffblk;
  char a[10] = " :\*.*";
  a[0]=drive+64;
  if (!findfirst(a,&ffblk,FA_LABEL)) {
   strcpy(label,ffblk.ff_name);
  } else {
   strcpy(label,"");
  }
 #endif
}

char *getIFSname(int drive)
{
   static char	ifs_name[256];

 #ifdef __OS2__
   char		devicename[3];
   ULONG        ordinal,infolevel;
   APIRET	rc;
   struct {
     USHORT a,b,c,d;
     CHAR data[9000];
   } Buffer;
   ULONG	BufferLen;

   devicename[0]=64+drive;
   devicename[1]=':';
   devicename[2]=0;
   infolevel=1;
   BufferLen=sizeof(Buffer);

   rc = DosQueryFSAttach(devicename,ordinal,infolevel,
			 (void*)&Buffer,&BufferLen);
   if (rc!=0) return 0;
   if (Buffer.c>sizeof(ifs_name)) Buffer.c=sizeof(ifs_name);
   memcpy(ifs_name,&Buffer.data[Buffer.b+1],Buffer.c+1);
   return ifs_name;
 #else
   // code for DOS
   struct fatinfo data;
   getfat(drive,&data);
   switch (data.fi_fatid) {
     case 0xfff0: strcpy(ifs_name,"other"); break;
     default: strcpy(ifs_name,"FAT");
   }
   // sprintf(ifs_name,"%x",data.fi_fatid);
   return ifs_name;
 #endif
}

#ifdef __MSDOS__
int DriveExists(unsigned char drive)
{
 struct fcb fcb;
 char a[20] = " :foo.oof";
 a[0]=drive+64;
 return (int)(parsfnm(a,&fcb,1));
}
#endif


int main(void)
{
  int maxdrives,curdrive, x;
  unsigned long disksize,diskfree,clustersize,sizesum=0,freesum=0;
  unsigned long capacity;
  char label[41],file_system[256];

  #ifdef __OS2__
   DosError(FERR_DISABLEHARDERR|FERR_ENABLEEXCEPTION);
  #endif

  printf("\nDF/" VERSION_BITS "bit v" VERSION_NO
	 "     Copyright (c) Timo Kokkonen, OH6LXV  "
	 "             " RELEASE_DATE "\n\n");

  curdrive=getdisk();
  maxdrives=setdisk(curdrive);
  if (maxdrives-2<=0) return 1;

  printf("disk               free       used      total capacity  cluster   file system\n");
  for(x=3;x<=maxdrives;x++) {
    label[0]=0;
    file_system[0]=0;
    #ifdef __MSDOS__
    if (!DriveExists(x)) continue;
    #endif
    if (getDiskSize(x,&disksize,&diskfree,&clustersize)) {
      getDiskLabel(x,label);
      strcpy(file_system,getIFSname(x));
    }
    sizesum+=disksize; freesum+=diskfree;
    if (disksize==0) capacity=0;
      else capacity=((disksize-diskfree)/(disksize/100));

    printf("%c:%-12s %8luk  %8luk  %8luk  %3lu%%  %5lu bytes   %-8s \n",
       x+64,label,diskfree/1024,(disksize-diskfree)/1024,disksize/1024,
       capacity,clustersize,file_system);
  }

  printf("\nTotal:        %9luk %9luk %9luk  %3lu%% \n",
     freesum/1024,(sizesum-freesum)/1024,sizesum/1024,
     (sizesum-freesum)/(sizesum/100));

  return 0;
}
