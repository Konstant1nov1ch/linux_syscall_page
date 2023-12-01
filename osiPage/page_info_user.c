#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <syscall.h>

#define BUFFER_SIZE 512

struct user_page {
    unsigned long flags;
    unsigned long vm_start;
};

struct user_vm_area_struct {
    unsigned long flags;
    unsigned long vm_start;
    unsigned long vm_end;
};

struct module_info {
    char name[BUFFER_SIZE];
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "2 аргумента требуются: {pid} struct_name={page | mode}\n");
        return 1;
    }

    int pid = atoi(argv[1]);
    char *struct_name = argv[2];
    int struct_id;

    struct user_page upage;
    struct user_vm_area_struct uvm_area_struct;
    struct module_info modules[BUFFER_SIZE];
    int module_count = 0;

    if (pid == 0 && !isdigit(argv[1][0])) {
        fprintf(stderr, "{pid} должен быть числом\n");
        return 1;
    }

    if (strcmp(struct_name, "page") == 0)
        struct_id = 0;
    else if (strcmp(struct_name, "mode") == 0)
        struct_id = 1;
    else {
        fprintf(stderr, "{struct_name} должно быть 'page' или 'mode'\n");
        return 1;
    }

    long result = syscall(335, pid, struct_id);

    if (result < 0) {
        perror("syscall");
        return 1;
    }

    if (struct_id == 0) {
        if (result == 0) {
            fprintf(stderr, "Ошибка: нулевая страница\n");
            return 1;
        }
        if (memcpy(&upage, (void *)result, sizeof(upage)) == NULL) {
            perror("copy_to_user");
            return 1;
        }
        printf("flags = %lu  vm_start = %lu\n", upage.flags, upage.vm_start);
    } else if (struct_id == 1) {
        if (result == 0) {
            fprintf(stderr, "Ошибка: нулевая страница\n");
            return 1;
        }
        if (memcpy(&uvm_area_struct, (void *)result, sizeof(uvm_area_struct)) == NULL) {
            perror("copy_to_user");
            return 1;
        }
        printf("flags = %lu  vm_start = %lu  vm_end = %lu\n",
               uvm_area_struct.flags, uvm_area_struct.vm_start, uvm_area_struct.vm_end);
    }

    result = syscall(336, modules, sizeof(modules));
    if (result < 0) {
        perror("syscall");
        return 1;
    }

    module_count = result / sizeof(struct module_info);

    printf("Загруженные модули:\n");
    for (int i = 0; i < module_count; i++) {
        int j;
        for (j = 0; j < i; j++) {
            if (strcmp(modules[i].name, modules[j].name) == 0) {
                break; 
            }
        }
        if (j == i) {
            printf("%s\n", modules[i].name);
        }
    }

    return 0;
}
