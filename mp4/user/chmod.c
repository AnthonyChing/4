#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/fs.h"

int chmod_recursive(char *path, int mode, int operation, int post_order);

// Open the directory and read all entries. chmod path/entry for all entries
void chmod_dir(char *path, int mode, int operation, int post_order){
    int fd, new_mode;
    struct stat st;
    char buf[512] = {0}; // Buffer to read directory entries
    char *p;

    // Open the directory
    if((fd = open(path, O_RESOLV)) < 0){
        fprintf(2, "chmod: cannot chmod %s\n", path);
        exit(1);
    }
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "chmod: cannot chmod %s\n", path);
        close(fd);
        exit(1);
    }
    close(fd);

    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    struct dirent de;

    if (operation == 1) // '+', add mode
        new_mode = st.mode | mode;
    else // '-', remove mode
        new_mode = st.mode & ~mode;
    
    if(post_order==0){
        chmod(path, new_mode);
    }
    if((fd = open(path, O_RDONLY)) < 0){
        fprintf(2, "chmod: cannot chmod %s\n", path);
        exit(1);
    }
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if (de.inum == 0)
            continue; // Skip empty entries
        if(de.name[0] == '.' && (de.name[1] == '\0' || (de.name[1] == '.' && de.name[2] == '\0')))
            continue; // Skip '.' and '..' entries
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0; // Null-terminate the path
        if (post_order) {
            // Recursively chmod the entry first
            chmod_recursive(buf, mode, operation, post_order);
        } else {
            // Change the mode of the entry directly
            chmod_recursive(buf, mode, operation, post_order);
        }
    }
    close(fd);
    if(post_order==1){
        chmod(path, new_mode);
    }
}

int chmod_recursive(char *path, int mode, int operation, int post_order){
    int fd, new_mode;
    struct stat st;
    // Check all parent directories' read permissions
    if((fd = open(path, O_NOACCESS)) < 0){
        fprintf(2, "chmod: cannot chmod %s\n", path, fd);
        exit(1);
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "chmod: cannot chmod %s\n", path);
        close(fd);
        exit(1);
    }
    close(fd);

    if(st.type == T_FILE){ // If it's a file, change the mode directly
        if (operation == 1) // '+', add mode
            new_mode = st.mode | mode;
        else // '-', remove mode
            new_mode = st.mode & ~mode;
        // omode flag doesn't matter because file type is file or directory
        chmod(path, new_mode);
    }
    else if(st.type == T_SYMLINK){
        // If it's a symbolic link, open it with O_RESOLV to resolve the link
        if((fd = open(path, O_RESOLV)) < 0){
            fprintf(2, "chmod: cannot chmod %s\n", path);
            exit(1);
        }
        if (fstat(fd, &st) < 0)
        {
            fprintf(2, "chmod: cannot chmod %s\n", path);
            close(fd);
            exit(1);
        }
        close(fd);
        if(st.type == T_FILE){
            if(operation == 1) // '+', add mode
                new_mode = st.mode | mode;
            else // '-', remove mode
                new_mode = st.mode & ~mode;
            chmod(path, new_mode);
        }
        else if(st.type == T_DIR){
            chmod_dir(path, mode, operation, post_order);
        }
    }
    else if(st.type == T_DIR){ // If it's a directory, recursively chmod all entries
        chmod_dir(path, mode, operation, post_order);
    }
    else{
        fprintf(2, "chmod: cannot chmod %s\n", path);
        exit(1);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    char path[MAXPATH];
    int mode;
    int new_mode;
    int operation;
    int recursive;
    int post_order; // 1 for post-order traversal, 0 for pre-order traversal
    struct stat st;
    int fd;
    if (argc < 3){
        fprintf(2, "Usage: chmod [-R] (+|-)(r|w|rw|wr) file_name|dir_name\n");
        exit(1);
    }
    if (argv[1][0] == '-' && argv[1][1] == 'R'){
        recursive = 1;
        if (argc < 4){
            fprintf(2, "Usage: chmod [-R] (+|-)(r|w|rw|wr) file_name|dir_name\n");
            exit(1);
        }
        strcpy(argv[1], argv[2]);
        strcpy(argv[2], argv[3]);
    }
    else{
        recursive = 0;
    }
    if(argv[1][0] == '+' && argv[1][1] == 'r' && argv[1][2] == '\0'){
        mode = M_READ;
        operation = 1;
    }
    else if(argv[1][0] == '+' && argv[1][1] == 'w' && argv[1][2] == '\0'){
        mode = M_WRITE;
        operation = 1;
    }
    else if(argv[1][0] == '+' && argv[1][1] == 'r' && argv[1][2] == 'w' && argv[1][3] == '\0'){
        mode = M_ALL;
        operation = 1;
    }
    else if(argv[1][0] == '+' && argv[1][1] == 'w' && argv[1][2] == 'r' && argv[1][3] == '\0'){
        mode = M_ALL;
        operation = 1;
    }
    else if(argv[1][0] == '-' && argv[1][1] == 'r' && argv[1][2] == '\0'){
        mode = M_READ;
        operation = 0;
    }
    else if(argv[1][0] == '-' && argv[1][1] == 'w' && argv[1][2] == '\0'){
        mode = M_WRITE;
        operation = 0;
    }
    else if(argv[1][0] == '-' && argv[1][1] == 'r' && argv[1][2] == 'w' && argv[1][3] == '\0'){
        mode = M_ALL;
        operation = 0;
    }
    else if(argv[1][0] == '-' && argv[1][1] == 'w' && argv[1][2] == 'r' && argv[1][3] == '\0'){
        mode = M_ALL;
        operation = 0;
    }
    else{
        fprintf(2, "Usage: chmod [-R] (+|-)(r|w|rw|wr) file_name|dir_name\n");
        exit(1);
    }
    strcpy(path, argv[2]);

    if(recursive == 1 && operation == 0 && (mode == M_READ || mode == M_ALL)){
        post_order = 1; // Post-order traversal
    }
    else{
        post_order = 0; // Pre-order traversal
    }

    if((fd = open(path, O_NOACCESS)) < 0){
        fprintf(2, "chmod: cannot chmod %s\n", path, fd);
        exit(1);
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "chmod: cannot chmod %s\n", path);
        close(fd);
        exit(1);
    }
    close(fd);

    if(recursive == 1){
        chmod_recursive(path, mode, operation, post_order);
    }
    else{
        // If it's a file or a directory, change the mode directly
        // If it's a symbolic link, call open with O_RESOLV and then change the mode directly
        if(st.type == T_FILE || st.type == T_DIR){
            if (operation == 1) // '+', add mode
                new_mode = st.mode | mode;
            else // '-', remove mode
                new_mode = st.mode & ~mode;
            // omode flag doesn't matter because file type is file or directory
            chmod(path, new_mode);
        }
        else if(st.type == T_SYMLINK){
            // If it's a symbolic link, open it with O_RESOLV to resolve the link
            if((fd = open(path, O_RESOLV)) < 0){
                fprintf(2, "chmod: cannot chmod %s\n", path);
                exit(1);
            }
            if (fstat(fd, &st) < 0)
            {
                fprintf(2, "chmod: cannot chmod %s\n", path);
                close(fd);
                exit(1);
            }
            close(fd);
            if (operation == 1) // '+', add mode
                new_mode = st.mode | mode;
            else // '-', remove mode
                new_mode = st.mode & ~mode;
            chmod(path, new_mode);
        }
        else{
            fprintf(2, "chmod: cannot chmod %s\n", path);
            exit(1);
        }
    }
    exit(0);
}
