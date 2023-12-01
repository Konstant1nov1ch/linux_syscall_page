#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define KBUF_SIZE 2048

// Функция для получения страницы по адресу виртуальной памяти в заданном mm_struct
struct page* get_page_by_mm_and_address(struct mm_struct* mm, long address) {
    pgd_t* pgd;
    p4d_t* p4d;
    pud_t* pud;
    pmd_t* pmd;
    pte_t* pte;
    struct page* page = NULL;

    pgd = pgd_offset(mm, address);
    if (!pgd_present(*pgd)) {
        return NULL;
    }

    p4d = p4d_offset(pgd, address);
    if (!p4d_present(*p4d)) {
        return NULL;
    }

    pud = pud_offset(p4d, address);
    if (!pud_present(*pud)) {
        return NULL;
    }

    pmd = pmd_offset(pud, address);
    if (!pmd_present(*pmd)) {
        return NULL;
    }

    pte = pte_offset_kernel(pmd, address);
    if (!pte_present(*pte)) {
        return NULL;
    }

    page = pte_page(*pte);
    if (page_to_pfn(page) == 0x10) {
        return NULL; // Пропуск адреса 0000000000000010
    }

    return page;
}

// Определение системного вызова
SYSCALL_DEFINE2(my_syscall, int, pid_input, int, struct_id) {
    struct pid *pid_struct;
    struct task_struct *task_struct;
    struct mm_struct *mm_struct;

    pid_struct = find_get_pid(pid_input);
    if (!pid_struct) {
        printk(KERN_INFO "Process with pid=%d doesn't exist\n", pid_input);
        return -EINVAL;
    }

    task_struct = pid_task(pid_struct, PIDTYPE_PID);
    if (!task_struct) {
        printk(KERN_INFO "Failed to get task_struct with pid=%d\n", pid_input);
        return -EINVAL;
    }

    mm_struct = task_struct->mm;
    if (!mm_struct) {
        printk(KERN_INFO "Failed to get mm_struct with pid=%d\n", pid_input);
        return -EINVAL;
    }

    struct vm_area_struct *vm_area_struct = mm_struct->mmap;
    unsigned long vm_start, vm_end;
    struct page *page;

    if (struct_id == 0) {
        vm_start = vm_area_struct->vm_start;
        vm_end = vm_area_struct->vm_end;
        while (vm_area_struct) {
            vm_area_struct = vm_area_struct->vm_next;
            for (; vm_start < vm_end; vm_start += PAGE_SIZE) {
                page = get_page_by_mm_and_address(mm_struct, vm_start);

                if (!page) {
                    //printk(KERN_INFO "Page not found for address: %lu\n", vm_start);
                    continue;
                }else{

                        printk(KERN_INFO "-----PAGE-----\nflags: %lu\n", page->flags);
                        printk(KERN_INFO "refcount:{ \n   counter: %u\n", page->_refcount.counter);
                        printk(KERN_INFO "}\n\n");
                }
            }
            vm_area_struct = vm_area_struct->vm_next;
        }
    } else if (struct_id == 1) {
        struct vm_area_struct *vm_area_struct;
        struct vm_area_struct *next_vm_area_struct = NULL;
        for (vm_area_struct = current->mm->mmap; vm_area_struct != NULL; vm_area_struct = next_vm_area_struct) {
            vm_start = vm_area_struct->vm_start;
            vm_end = vm_area_struct->vm_end;
            next_vm_area_struct = vm_area_struct->vm_next;
            for (; vm_start < vm_end; vm_start += PAGE_SIZE) {
                if (vm_area_struct->vm_file) {
                    char buf[256];
                    char last_name[256];
                    struct file *file = vm_area_struct->vm_file;
                    char *name = d_path(&file->f_path, buf, sizeof(buf));
                    if (!IS_ERR(name) && strcmp(last_name, name) != 0) {
                        strcpy(last_name, name); // Сохраняем текущее имя для сравнения с следующим
                        printk(KERN_INFO "Module: %s\n", name);
                    }

                }
            }
        }
    }



    return 0;
}
