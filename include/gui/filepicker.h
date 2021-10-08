#ifndef FILEPICKER_H
#define FILEPICKER_H
struct filepicker;
typedef bool(*filepicker_callback)(bool ok,const char**path,uint16_t cnt,void*user_data);
extern struct filepicker*filepicker_create(filepicker_callback callback,const char*title,...) __attribute__((format(printf,2,3)));
extern void filepicker_set_title(struct filepicker*fp,const char*title,...) __attribute__((format(printf,2,3)));
extern void filepicker_set_path(struct filepicker*fp,const char*path,...) __attribute__((format(printf,2,3)));
extern void filepicker_set_user_data(struct filepicker*fp,void*user_data);
extern void filepicker_set_max_item(struct filepicker*fp,uint16_t max);
#endif
