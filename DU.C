/**************************************************
* DU (*nix like) for OS/2 v2.x
* Copyright (c) 1993,1994 Timo Kokkonen.
* (c) 1994 Julien Pierre.
*
* compiler: IBM C Set ++ for OS/2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __OS2__
#include <os2.h>
#else
#include <dos.h>
#endif

 #define VERSION_BITS "32"
 #define FILEMASK "*"

#define RELEASE_DATE "01-Jan-94"
#define MODIF_DATE "04-Sep-94"
#define VERSION_NO   "1.11"

/******************** GLOBAL VARIABLES *********************/
int level=0;
int sublevels=1;
unsigned long cluster_size;
unsigned long fat_cluster_size;
unsigned long hpfs_cluster_size=512;
unsigned long true_sum=0;
unsigned long fat_sum=0;
unsigned long hpfs_sum=0;
unsigned long file_counter=0;
char ifs_name[256];

/***********************************************************/

void print_syntax(void)
{
  printf("\nSyntax: DU [directory] [/n]\n\n"
	 "  directory = pathname of base-directory\n"
	 "  n         = how many sub-directory levels to display,\n"
	 "              default = 1 (0 = all)\n\n");

}


char *SplitDir(char *pathname)
{
  static char dirri[_MAX_PATH];
  char drive[_MAX_DRIVE],
       dir[_MAX_DIR],
       file[_MAX_FNAME],
       ext[_MAX_EXT];

  _splitpath(pathname,drive,dir,file,ext);
  strcpy(dirri,drive);
  strcat(dirri,dir);
  return dirri;
}


int getDiskSize(unsigned char drive,unsigned long *size,
				    unsigned long *free,
				    unsigned long *clustersize,
                                    unsigned long *fc)
{
    typedef struct diskdata {
                ULONG filesysid, sectornum, unitnum, unitavail, bytesnum;
                                 } diskdata;
    struct diskdata FSInfoBuf;
    ULONG DriveNumber;
    ULONG FSInfoLevel;
    ULONG FSInfoBufSize;
    ULONG fatsectornum;
    ULONG Mo = 1024*1024;
    APIRET rc;
    char disque [3];
    struct {
      USHORT a,b,c,d;
      char data[9000];
    } Buffer;
    ULONG BufferLen, infolevel, ordinal;

  memset(&FSInfoBuf,0,sizeof(diskdata));
  DriveNumber = drive;
  FSInfoLevel = FSIL_ALLOC;
  FSInfoBufSize = sizeof( diskdata );
  if (DosQueryFSInfo(DriveNumber, FSInfoLevel, (PVOID) &FSInfoBuf, FSInfoBufSize) == 0)
{
    *size=(unsigned long)FSInfoBuf.unitnum*FSInfoBuf.bytesnum*
	  FSInfoBuf.sectornum;

    fatsectornum = 0;
    if (*size <2048*Mo) fatsectornum = 64;
    if (*size <1024*Mo) fatsectornum = 32;
    if (*size <512*Mo) fatsectornum = 16;
    if (*size <256*Mo) fatsectornum = 8;
    if (*size <128*Mo) fatsectornum = 4;
    if (*size <64*Mo) fatsectornum = 2;
    if (*size <32*Mo) fatsectornum = 1;

    *free=(unsigned long)FSInfoBuf.unitavail*FSInfoBuf.bytesnum*
	  FSInfoBuf.sectornum;
    *clustersize=(unsigned long)FSInfoBuf.bytesnum*
		 FSInfoBuf.sectornum;
    *fc=(unsigned long)512*fatsectornum;
    BufferLen = sizeof(Buffer);
    disque[0] = 64+drive;
    disque[1] = ':';
    disque[2] = 0;
    infolevel = 1;
    ordinal = 0;
    rc = DosQueryFSAttach(&disque,ordinal,infolevel,
         (void*)&Buffer,&BufferLen);
    if (rc == 0) {
                   if (Buffer.c>sizeof(ifs_name)) Buffer.c=sizeof(ifs_name);
                   memcpy(ifs_name,&Buffer.data[Buffer.b+1],Buffer.c+1);
                 }
           else
                 ifs_name[0] = 0;
    return 1;
  }
  *size=*free=*clustersize=0;
  return 0;
}


unsigned long calc_dir(char *pathname)
{
  char apu[_MAX_PATH];
  int done;
  unsigned long sum=0;
  ULONG count;
  HDIR findhandle;
  FILEFINDBUF3 resbuf;

  level++;
  strcpy(apu,pathname);
  strcat(apu,FILEMASK);
  count = 1;
  findhandle = HDIR_CREATE;
  done= DosFindFirst(apu,&findhandle,FILE_ARCHIVED + FILE_DIRECTORY +
         FILE_SYSTEM + FILE_HIDDEN + FILE_READONLY,&resbuf,
         sizeof(FILEFINDBUF3),&count,FIL_STANDARD);
  while (!done) {
    sum+=resbuf.cbFile;
    if (resbuf.cbFile % cluster_size == 0) true_sum+=resbuf.cbFile;
     else true_sum+=cluster_size+resbuf.cbFile-(resbuf.cbFile%cluster_size);
    if (resbuf.cbFile % fat_cluster_size == 0) fat_sum+=resbuf.cbFile;
     else fat_sum += fat_cluster_size+resbuf.cbFile-(resbuf.cbFile%fat_cluster_size);
    if (resbuf.cbFile % hpfs_cluster_size == 0) hpfs_sum+=resbuf.cbFile;
     else hpfs_sum += hpfs_cluster_size+resbuf.cbFile-(resbuf.cbFile%hpfs_cluster_size);
    if (!(resbuf.attrFile&FILE_DIRECTORY)) file_counter++;
    if ((resbuf.attrFile  & FILE_DIRECTORY)&& (strcmp(resbuf.achName,".."))&&
      (strcmp(resbuf.achName,"."))) {
      strcpy(apu,pathname);
      strcat(apu,resbuf.achName);
      strcat(apu,"\\");
      sum+=calc_dir(apu);
    }
    count = 1;
    done= DosFindNext(findhandle,&resbuf,sizeof(FILEFINDBUF3),&count);
  }
 if ((level<=sublevels+1)||(sublevels<=0)) printf("%7luk %s\n",sum/1024,pathname);
 level--;
 DosFindClose(findhandle);
 return sum;
};

void comparaison (unsigned long supposition, unsigned long reel)
{
   double diff;

   if (supposition != 0 )
   if (supposition>reel) 
                {
                   diff = (double) (supposition-reel) / supposition * 100.0;
/*                   printf("Soit %lu octets en moins, ou encore %3.1lf%% de capacit‚ disque en moins.\n",
                                                        supposition-reel,
                                                        diff); */
                   printf("Or %lu less bytes, or %3.1lf%% less capacity.\n",
                                                        supposition-reel,
                                                        diff);

                } 
                else 
                {
                   diff = (double) (reel-supposition) / supposition * 100.0;
/*                   printf("Soit %lu octets en plus, ou encore %3.1lf%% de capacit‚ disque en plus.\n",
                                                        reel-supposition,
                                                        diff); */
                   printf("Or %lu more bytes, or %3.1lf%% more capacity.\n",
                                                        reel-supposition,
                                                        diff);
                }; /* endif */
};
/*************** MAIN PROGRAM ******************************/
int main(int arg_count, char **args)
{
  char startpath[_MAX_PATH], *apu;
  char *defaultdir = ".";
  unsigned long disk_size,disk_free, summa;
  double slack,fat_slack,diff,hpfs_slack;

  printf("\nDU/" VERSION_BITS "bit v" VERSION_NO
	 "     Copyright (c) Timo Kokkonen, OH6LXV               "
	 RELEASE_DATE "\n");
  printf("                   Modified by Julien Pierre                         "
	 MODIF_DATE "\n\n");

  strcpy(startpath,".");
  if (arg_count>1) {
    if (!strcmp(args[1],"/?")) {
      print_syntax();
      exit(0);
    }
    if (args[arg_count-1][0]=='/')
     if (sscanf(&args[arg_count-1][1],"%d",&sublevels)!=1) {
       printf("\nInvalid parameter '%s'.\n",args[arg_count-1]);
       print_syntax();
       exit(1);
     }
  }
  if ((arg_count>=2)&&(args[1][0]!='/'))  apu=args[1];
    else apu=defaultdir;

  if (!_fullpath(startpath,apu,_MAX_PATH)) {
    printf("Cannot convert '%s' to absolute path.\n",apu);
    exit(1);
  }
  if (startpath[strlen(startpath)-1]!='\\') strcat(startpath,"\\");
  strcpy(startpath,SplitDir(startpath));

  if (!getDiskSize(startpath[0]-64,&disk_size,&disk_free,&cluster_size,&fat_cluster_size)) {
    printf("Cannot read drive %c: \n",startpath[0]);
    exit(1);
  }
  summa=calc_dir(startpath);
  if (true_sum!=0) slack=(1-(double)summa/true_sum)*100.0;
    else slack=0;
  if (fat_sum!=0) fat_slack=(1-(double)summa/fat_sum)*100.0;
    else fat_slack=0;
  if (hpfs_sum != 0) hpfs_slack=(1-(double)summa/hpfs_sum)*100.0;
    else hpfs_slack=0;

  /* printf(" %lu  %lu  %lu \n",disk_size-disk_free,summa,true_sum); */
/*  printf("\n %lu octets dans %lu fichier(s), %3.1lf%% de gachis (%lu octets).\n",
							true_sum,
							file_counter,
							slack,
							true_sum-summa); */
  printf("\n %lu bytes in %lu file(s), %3.1lf%% of waste (%lu bytes).\n",
							true_sum,
							file_counter,
							slack,
							true_sum-summa);

/*  printf("\nCette partition utilise le systŠme de fichiers %s.\n",ifs_name);
  printf("Elle a une capacit‚ de %lu octets.\n",disk_size);
  printf("La taille des clusters est de %lu octets.\n",cluster_size); */
  printf("\nThis partition uses the %s file system.\n",ifs_name);
  printf("Its capacity is %lu bytes.\n",disk_size);
  printf("Cluster size is %lu bytes.\n",cluster_size);

  if ((strcmp(ifs_name,"FAT") != 0) && (fat_cluster_size !=0)) {
/*   printf("\nSi cette partition ‚tait format‚e avec le systŠme de fichiers FAT, la taille\ndes clusters serait de %lu octets et vous auriez\n",fat_cluster_size);
   printf("%lu octets dans %lu fichier(s), %3.1lf%% de gachis (%lu octets).\n",
                                fat_sum,
                                file_counter,
                                fat_slack,
                                fat_sum-summa); */
   printf("\nIf this partition was formatted with the FAT file system,\ncluster size would be %lu bytes and you would have\n",fat_cluster_size);
   printf("%lu bytes in %lu file(s), %3.1lf%% of waste (%lu bytes).\n",
                                fat_sum,
                                file_counter,
                                fat_slack,
                                fat_sum-summa);

   comparaison(fat_sum,true_sum);
  };
  if (strcmp(ifs_name,"HPFS") != 0) {
/*   printf("\nSi cette partition ‚tait format‚e avec le systŠme de fichiers HPFS, la taille\ndes clusters serait de %lu octets et vous auriez\n",hpfs_cluster_size);
   printf("%lu octets dans %lu fichier(s), %3.1lf%% de gachis (%lu octets).\n",
       hpfs_sum,
       file_counter,
       hpfs_slack,
       hpfs_sum-summa); */
   printf("\nIf this partition was formatted with the HPFS file system,\ncluster size would be %lu bytes and you would have\n",hpfs_cluster_size);
   printf("%lu bytes in %lu file(s), %3.1lf%% of waste (%lu bytes).\n",
       hpfs_sum,
       file_counter,
       hpfs_slack,
       hpfs_sum-summa);
   comparaison(hpfs_sum,true_sum);
  };

  return 0;
};
