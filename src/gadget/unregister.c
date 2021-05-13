#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
#include"system.h"
#include"logger.h"
#include"gadget.h"
#define TAG "gadget"

int gadget_unregister_all(){
	int o=open_usb_gadget();
	if(o<0)return o;
	int r=gadget_unregister_all_fd(o);
	close(o);
	return r;
}

int gadget_unregister(char*name){
	int o=open_usb_gadget();
	if(o<0)return o;
	int r=gadget_unregister_fd(o,name);
	close(o);
	return r;
}

int gadget_unregister_fd(int dir,char*name){
	int d=-1,cs=-1,fs=-1,ss=-1;
	if(!(
		dir>=0&&name&&
		(d=openat(dir,name,O_RDONLY|O_DIRECTORY))>=0&&
		(cs=openat(d,"configs",O_RDONLY|O_DIRECTORY))>=0&&
		(fs=openat(d,"functions",O_RDONLY|O_DIRECTORY))>=0&&
		(ss=openat(d,"strings",O_RDONLY|O_DIRECTORY))>=0
	))goto er;
	gadget_stop_fd(d);
	DIR*subdir;
	struct dirent*rent;
	if(!(subdir=fdopendir(cs)))goto er;
	while((rent=readdir(subdir)))if(!is_virt_dir(rent)&&rent->d_type==DT_DIR){
			int cfg,str;
			if((cfg=openat(cs,rent->d_name,O_RDONLY|O_DIRECTORY))<0)continue;
			if(remove_folders(cfg,0)<0){
				close(cfg);
				continue;
			}
			if((str=openat(cfg,"strings",O_RDONLY|O_DIRECTORY))<0)continue;
			if(remove_folders(cfg,AT_REMOVEDIR)<0){
				close(str);
				close(cfg);
				continue;
			}
			close(str);
			close(cfg);
			unlinkat(cs,rent->d_name,AT_REMOVEDIR);
		}
	closedir(subdir);
	if(remove_folders(fs,AT_REMOVEDIR)<0)goto er;
	if(remove_folders(ss,AT_REMOVEDIR)<0)goto er;
	close(d);
	int z=unlinkat(dir,name,AT_REMOVEDIR);
	tlog_info("unregister %s",name);
	return z;
	er:
	telog_error("unregister %s failed",name);
	if(cs>=0)close(cs);
	if(fs>=0)close(fs);
	if(ss>=0)close(ss);
	if(d>=0)close(d);
	return -1;
}

int gadget_unregister_all_fd(int dir){
	DIR*f=NULL;
	if(!(dir>=0&&(f=fdopendir(dir))))return -1;
	struct dirent*x;
	while((x=readdir(f)))
		if(!is_virt_dir(x)&&x->d_type==DT_DIR)
			gadget_unregister_fd(dir,x->d_name);
	free(f);
	return 0;
}
