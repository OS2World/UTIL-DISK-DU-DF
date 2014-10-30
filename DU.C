/**************************************************
* DU (*nix like) for OS/2 v2.x & DOS
* Copyright (c) 1993,1994 Timo Kokkonen.
*
* compiler: Borland C++ for OS/2 or DOS
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>


#ifdef __OS2__
 #define VERSION_BITS "32"
 #define FILEMASK "*"
#else
 #define VERSION_BITS "16"
 #define FILEMASK "*.*"
#endif

#define RELEASE_DATE "01-Jan-94"
#define VERSION_NO   "1.11"


/******************** GLOBAL VARIABLES *********************/
int level=0;
int sublevels=1;
unsigned long cluster_size;
unsigned long true_sum=0;
unsigned long file_counter=0;

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


unsigned long calc_dir(char *pathname)
{
  struct find_t  direntry;
  char apu[_MAX_PATH];
  int done;
  unsigned long sum=0;

  level++;
  strcpy(apu,pathname);
  strcat(apu,FILEMASK);
  done=_dos_findfirst(apu,_A_NORMAL+_A_RDONLY+_A_HIDDEN+_A_SYSTEM+
			  _A_SUBDIR+_A_ARCH,&direntry);
  while (!done) {
    sum+=direntry.size;
    if (direntry.size % cluster_size == 0) true_sum+=direntry.size;
     else true_sum+=cluster_size+direntry.size-(direntry.size%cluster_size);
    if (!(direntry.attrib&_A_SUBDIR)) file_counter++;
    if ((direntry.attrib  & _A_SUBDIR)&& (strcmp(direntry.name,".."))&&
	(strcmp(direntry.name,"."))) {
      strcpy(apu,pathname);
      strcat(apu,direntry.name);
      strcat(apu,"\\");
      sum+=calc_dir(apu);
    }
    done=_dos_findnext(&direntry);
  }
 if ((level<=sublevels+1)||(sublevels<=0)) printf("%7luk %s\n",sum/1024,pathname);
 level--;
 return sum;
}


/*************** MAIN PROGRAM ******************************/
int main(int arg_count, char **args)
{
  char startpath[_MAX_PATH], *apu;
  char *defaultdir = ".";
  unsigned long disk_size,disk_free, summa;
  double slack;

  printf("\nDU/" VERSION_BITS "bit v" VERSION_NO
	 "     Copyright (c) Timo Kokkonen, OH6LXV               "
	 RELEASE_DATE "\n\n");

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

  if (!getDiskSize(startpath[0]-64,&disk_size,&disk_free,&cluster_size)) {
    printf("Cannot read drive %c: \n",startpath[0]);
    exit(1);
  }
  summa=calc_dir(startpath);
  if (true_sum!=0) slack=(1-(double)summa/true_sum)*100.0;
    else slack=0;

  // printf(" %lu  %lu  %lu \n",disk_size-disk_free,summa,true_sum);
  printf("\n %lu bytes in %lu file(s), %1.1lf%% slack (%lu bytes).\n",
							true_sum,
							file_counter,
							slack,
							true_sum-summa);
  return 0;
}
