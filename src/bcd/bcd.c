/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<hivex.h>
#include<fcntl.h>
#include<stdio.h>
#include<dirent.h>
#include<stdarg.h>
#include<string.h>
#include<signal.h>
#include<endian.h>
#include<limits.h>
#include<unistd.h>
#include<stdbool.h>
#include<uuid/uuid.h>
#include<blkid/blkid.h>
#include<sys/stat.h>
#include<sys/sysmacros.h>
#include"keyval.h"
#include"bcdstore.h"
hive_h*bcd;
hive_node_h root,obj,bootmgr,def_boot;
device_t osdev,appdev;
static int reterr(int e,const char*c){
	fputs(c,stderr);
	fprintf(stderr,errno>0?": %m\n":".\n");
	return e;
}
static _Noreturn void failure(){
	fputs("internal data failure\n",stderr);
	raise(SIGABRT);
	exit(2);
}
char*force_get(keyval**arr,char*key,char*def){
	char*b=kvarr_get_value_by_key(arr,key,def);
	if(!b&&!def)failure();
	return b?b:def;
}
char*force_rget(keyval**arr,char*key,char*def){
	char*b=kvarr_get_key_by_value(arr,key,def);
	if(!b)failure();
	if(!b&&!def)failure();
	return b?b:def;
}
char*force_iget(keyval**arr,int key,char*def){
	char b[BUFSIZ]={0};
	snprintf(b,BUFSIZ-1,"%d",key);
	return force_get(arr,b,def);
}
char*force_riget(keyval**arr,int key,char*def){
	char b[BUFSIZ]={0};
	snprintf(b,BUFSIZ-1,"%d",key);
	return force_rget(arr,b,def);
}
#define fget_define(_s,_l) \
	static inline char*fget_##_s(char*key,char*def){return force_get((_l),key,def);}\
	static inline char*frget_##_s(char*key,char*def){return force_rget((_l),key,def);}\
	static inline char*figet_##_s(int key,char*def){return force_iget((_l),key,def);}\
	static inline char*friget_##_s(int key,char*def){return force_riget((_l),key,def);}
fget_define(app,BcdApplications);
fget_define(osl,BcdOSLoader);
fget_define(lib,BcdLibrary);
fget_define(mgr,BcdBootMgr);
fget_define(dep,DeviceType);
fget_define(ldp,LocalDeviceType);
hive_value_h bcd_get_value(hive_node_h node,char*key){
	hive_node_h k,e;
	if((e=hivex_node_get_child(bcd,node,"Elements"))<=0)return 0;
	if((k=hivex_node_get_child(bcd,e,key))<=0)return 0;
	return hivex_node_get_value(bcd,k,"Element");
}
void bcd_out_strval(hive_node_h node,char*prefix,char*key){
	char*str;
	hive_value_h val;
	if((val=bcd_get_value(node,key))<=0)return;
	if(!(str=hivex_value_string(bcd,val)))return;
	if(prefix)fputs(prefix,stdout);
	puts(str);
	free(str);
}
void bcd_xout_strval(hive_node_h node,keyval**kv,char*key){
	char*str=NULL,desc[BUFSIZ]={0};
	hive_value_h val;
	snprintf(desc,BUFSIZ-1,"%s: ",key);
	bcd_out_strval(node,desc,force_get(kv,key,NULL));
}
hive_node_h get_default_boot(){
	hive_value_h d;
	hive_node_h n;
	char*name;
	if((d=bcd_get_value(bootmgr,fget_mgr("DefaultObject",NULL)))<=0)return 0;
	if(!(name=hivex_value_string(bcd,d)))return 0;
	n=hivex_node_get_child(bcd,obj,name);
	free(name);
	return n;
}
int init_bcd(const char*path,int flags){
	if(!(bcd=hivex_open(path,flags)))return reterr(0,"open");

	if((root=hivex_root(bcd))<=0)
		return reterr(1,"failed to get root node");

	if((obj=hivex_node_get_child(bcd,root,"Objects"))<=0)
		return reterr(1,"failed to get objects node");

	if((bootmgr=hivex_node_get_child(bcd,obj,fget_app("BOOTMGR",NULL)))<=0)
		return reterr(1,"failed to get bootmgr node");
	return 0;
}
device_t*get_device(device_t*dev,char*key,hive_node_h node){
	hive_value_h x;
	hive_type t;
	size_t s=0;
	char*d;
	if(!(x=bcd_get_value(node,key)))return NULL;
	if(!(d=hivex_value_value(bcd,x,&t,&s)))return NULL;
	if(t!=hive_t_binary||s!=sizeof(device_t))return NULL;
	memcpy(dev,d,sizeof(device_t));
	free(d);
	return dev;
}
ssize_t read_file(char*buff,size_t len,bool lf,char*path,...){
        int fd;
        va_list va;
        char rpath[PATH_MAX]={0};
        if(!path||!buff||len<=0)return -(errno=EINVAL);
        va_start(va,path);
        vsnprintf(rpath,PATH_MAX-1,path,va);
        va_end(va);
        memset(buff,0,len);
        if((fd=open(rpath,O_RDONLY))<0)return -1;
        ssize_t s=read(fd,buff,len-1);
        if(s>0&&!lf){
                if(buff[s-1]=='\n')buff[--s]=0;
                if(buff[s-1]=='\r')buff[--s]=0;
        }
        close(fd);
        return s;
}
char*get_kname(char*path){
	struct stat st;
	if(stat(path,&st)<0)return NULL;
	if(!S_ISBLK(st.st_mode))return NULL;
	char p[PATH_MAX],*pp=p;
	if(read_file(
		p,PATH_MAX,true,
		"/sys/dev/block/%d:%d/uevent",
		major(st.st_rdev),
		minor(st.st_rdev)
	)<0)return NULL;
	while(true){
		char*key=pp,*val;
		if(!(val=strchr(key,'=')))break;
		*(val++)=0;
		if(!(pp=strchr(val,'\n')))break;
		*(pp++)=0;
		if(strcmp(key,"DEVNAME")==0)return strdup(val);
	}
	return NULL;
}
char*get_pkname(char*kname){
	char*pkname=NULL;
	struct dirent*n;
	struct stat st;
	char p[BUFSIZ];
	DIR*d;
	if(!(d=opendir("/sys/block")))goto e;
	while((n=readdir(d))){
		if(n->d_type!=DT_LNK||n->d_name[0]=='.')continue;
		memset(p,0,BUFSIZ);
		snprintf(p,BUFSIZ-1,"/sys/block/%s/%s/partition",n->d_name,kname);
		if(stat(p,&st)<0||!S_ISREG(st.st_mode))continue;
		pkname=strdup(n->d_name);
		break;
	}
	e:
	if(d)closedir(d);
	return pkname;
}
uuid_t*get_uuid(uuid_t uuid,const char*key,const char*path){
	static blkid_cache cache=NULL;
        if(!cache)blkid_get_cache(&cache,NULL);
	char*r=blkid_get_tag_value(cache,key,path);
	if(!r||uuid_parse(r,uuid)!=0)return NULL;
	free(r);
	return (uuid_t*)uuid;
}
void puts_guid(char*prefix,guid_t guid){
	uuid_t uu;
	char u[UUID_STR_LEN];
	guid2uuid(&uu,guid);
	uuid_unparse(uu,u);
	if(prefix)printf(prefix);
	puts(u);
}
void out_device(char*msg,device_t*dev){
	puts(msg);
	fputs("\tDevice Type: ",stdout);
	puts(friget_dep(dev->dev_type,"Unknown"));
	fputs("\tLocalDevice Type: ",stdout);
	puts(friget_ldp(dev->local_dev_type,"Unknown"));
	puts_guid("\tDisk UUID: ",dev->disk_guid);
	puts_guid("\tPartition UUID: ",dev->part_guid);
}
char*find_block(char*kname){
	static const char*map[]={
		"/dev",
		"/dev/disk",
		"/dev/block",
		NULL
	};
	struct stat st;
	size_t s=16+strlen(kname);
	char*block=malloc(s);
	if(!block)return NULL;
	for(int i=0;map[i];i++){
		memset(block,0,s);
		snprintf(block,s-1,"%s/%s",map[i],kname);
		if(stat(block,&st)<0||!S_ISBLK(st.st_mode))continue;
		return block;
	}
	free(block);
	return NULL;
}
int dump_info(){
	if(!def_boot){
		if((def_boot=get_default_boot())<=0)
			return reterr(1,"failed to get default boot entry");
		if(!get_device(&osdev,fget_osl("OSDevice",NULL),def_boot))
			return reterr(1,"failed to get osdevice");
		if(!get_device(&appdev,fget_lib("ApplicationDevice",NULL),def_boot))
			return reterr(1,"failed to get appdevice");
	}
	char*name=hivex_node_name(bcd,def_boot);
	if(name){
		fputs("Name: ",stdout);
		puts(name);
		free(name);
	}
	bcd_xout_strval(def_boot,BcdLibrary,"Description");
	bcd_xout_strval(def_boot,BcdLibrary,"ApplicationPath");
	bcd_xout_strval(def_boot,BcdLibrary,"PreferredLocale");
	bcd_xout_strval(def_boot,BcdOSLoader,"SystemRoot");
	bcd_xout_strval(def_boot,BcdOSLoader,"KernelPath");
	out_device("OsDevice:",&osdev);
	out_device("ApplicationDevice:",&appdev);
}
int set_device(hive_node_h node,char*key,device_t*val){
	hive_node_h e,k;
	if((e=hivex_node_get_child(bcd,node,"Elements"))<=0)return 0;
	if((k=hivex_node_get_child(bcd,e,key))<=0)return 0;
	return hivex_node_set_value(bcd,k,&(hive_set_value){
		.key="Element",
		.len=sizeof(device_t),
		.t=hive_t_binary,
		.value=(char*)val
	},0);
}
int process_change(char*part){
	uuid_t ku,pku;
	guid_t kg,pkg;
	char*kname,*pkname,*kpath,*pkpath;
	dump_info();
	putchar('\n');
	puts("Resolve block...");
	if(!(kname=get_kname(part)))return reterr(1,"failed to get block kernel name");
	if(!(pkname=get_pkname(kname)))return reterr(1,"failed to get parent block kernel name");
	if(!(kpath=find_block(kname)))return reterr(1,"failed to get block kernel path");
	if(!(pkpath=find_block(pkname)))return reterr(1,"failed to get parent block kernel path");
	puts("Read UUID...");
	if(!get_uuid(ku,"PARTUUID",kpath))return reterr(1,"failed to get PARTUUID");
	if(!get_uuid(pku,"PTUUID",pkpath))return reterr(1,"failed to get PTUUID");
	if(!uuid2guid(&pkg,pku)||!uuid2guid(&kg,ku))return reterr(1,"convert guid failed");
	puts("Change GUID...");
	memcpy(&appdev.disk_guid,&pkg,sizeof(guid_t));
	memcpy(&appdev.part_guid,&kg,sizeof(guid_t));
	memcpy(&osdev.disk_guid,&pkg,sizeof(guid_t));
	memcpy(&osdev.part_guid,&kg,sizeof(guid_t));
	puts("Write changes...");
	if(set_device(def_boot,fget_osl("OSDevice",NULL),&osdev)<0)
		return reterr(1,"set osdevice");
	if(set_device(def_boot,fget_lib("ApplicationDevice",NULL),&appdev)<0)
		return reterr(1,"set appdevice");
	if(hivex_commit(bcd,NULL,0)<0)
		return reterr(1,"commit changes");
	puts("Done");
	putchar('\n');
	dump_info();
	free(kname);
	free(pkname);
	free(kpath);
	free(pkpath);
	return 0;
}
int main(int argc,char**argv){
	if(argc!=3){
		fprintf(stderr,"Usage: %s <BCD> <BLOCK>\n",argv[0]);
		fputs("Change BCD default entry boot block\n",stderr);
		return 1;
	}
	if(init_bcd(argv[1],HIVEX_OPEN_WRITE)!=0)return 1;
	int r=process_change(argv[2]);
	hivex_close(bcd);
	return r;
}
