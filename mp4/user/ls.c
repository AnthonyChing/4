#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"

void mode_to_string(char *mode, int m)
{
    if (m == M_READ)
        strcpy(mode, "r-");
    else if (m == M_WRITE)
        strcpy(mode, "-w");
    else if (m == M_ALL)
        strcpy(mode, "rw");
    else
        strcpy(mode, "--");
    return;
}

char *fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

void read_dir(int fd, char *mode, char *buf, char *path){
    char *p;
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    struct dirent de;
    while (read(fd, &de, sizeof(de)) == sizeof(de)){
        if (de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        int file_fd;
        // Should use O_NOACCESS, because the permissions of the file doesn't matter and we don't want to follow any symlinks
        // printf("[read_dir] Reading %s\n", buf);
        if((file_fd = open(buf, O_NOACCESS)) < 0){
            printf("ls: cannot open %s\n", buf);
            close(fd);
            return;
        }
        struct stat file_st;
        if (fstat(file_fd, &file_st) < 0)
        {
            printf("ls: cannot stat %s\n", buf);
            close(file_fd);
            return;
        }
        close(file_fd);
        // printf("[read_dir] File type: %d, Inode: %d, Size: %d\n", file_st.type, file_st.ino, file_st.size);
        mode_to_string(mode, file_st.mode);
        printf("%s %d %d %d %s\n", fmtname(buf), file_st.type, file_st.ino, file_st.size, mode);
    }
    return;
}

/* TODO: Access Control & Symbolic Link */
void ls(char *path)
{
    char buf[512];
    int fd;
    struct stat st;

    if ((fd = open(path, O_NOACCESS)) < 0)
    {
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }
    close(fd);
    char mode[3];
    switch (st.type)
    {
    case T_FILE:
        if ((fd = open(path, O_RDONLY)) < 0)
        {
            fprintf(2, "ls: cannot open %s\n", path);
            return;
        }

        if (fstat(fd, &st) < 0)
        {
            fprintf(2, "ls: cannot stat %s\n", path);
            close(fd);
            return;
        }
        close(fd);
        mode_to_string(mode, st.mode);
        printf("%s %d %d %l %s\n", fmtname(path), st.type, st.ino, st.size, mode);
        return;

    case T_DIR:
        if((fd = open(path, O_RDONLY)) < 0){
            printf("ls: cannot open %s\n", path);
            close(fd);
            return;
        }
        read_dir(fd, mode, buf, path);
        close(fd);
        return;

    case T_SYMLINK:
        // printf("[ls] %s is a symbolic link\n", path);
        if((fd = open(path, O_RESOLV)) < 0){
            printf("ls: cannot open %s\n", path);
            close(fd);
            return;
        }
        struct stat st_symlink;
        if (fstat(fd, &st_symlink) < 0)
        {
            printf("ls: cannot stat %s\n", path);
            close(fd);
            return;
        }
        close(fd);
        if(st_symlink.type == T_FILE){
            mode_to_string(mode, st.mode);
            printf("%s %d %d %d %s\n", fmtname(path), st.type, st.ino, st.size, mode);
        }
        else if(st_symlink.type == T_DIR){
            // printf("[ls] %s points to a directory, reading contents...\n", path);
            if((fd = open(path, O_RDONLY)) < 0){
                printf("ls: cannot open %s\n", path);
                close(fd);
                return;
            }
            read_dir(fd, mode, buf, path);
            close(fd);
        }
        return;
    }

    return;
}

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2)
    {
        ls(".");
        exit(0);
    }
    for (i = 1; i < argc; i++)
        ls(argv[i]);
    exit(0);
}
