//arch/riscv/kernel/proc.c
#include "proc.h"
#include "defs.h"
#include "mm.h"
#include "fs.h"
#include "elf.h"
extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
extern unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));


struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组，所有的线程都保存在此

void dummy() {
    uint64 MOD = 1000000007;
    //uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            //auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running! thread space begin at 0x%lx\n", current->pid, &(*current)); 
        }
    }
}

//DSJF
#ifdef SJF
void schedule(void) {
    //if(current==task[1]) fs_test();
    int i;
    struct task_struct* next=NULL;
    for(i = 1; i < NR_TASKS && task[i]!=NULL; i++){
        if(task[i]->counter > 0 && task[i]->state == TASK_RUNNING){
            next = task[i];
            break;
        }
    }
    if(i==NR_TASKS || task[i]==NULL){
    	//printk("\n");
        for(i = 1; i < NR_TASKS && task[i]!=NULL; i++){
            task[i]->counter = rand();
            //task[i]->counter = 1;
            //task[i]->thread.ra=__dummy;
            //printk("TASK[%d]->pid = %d\n",i,task[i]->pid);
            //printk("SET [PID = %d COUNTER = %d]\n", task[i]->pid, task[i]->counter);
        }
    }
    for(i = 1; i < NR_TASKS && task[i]!=NULL; i++)
    {
        if(next==NULL&&task[i]->state==TASK_RUNNING || task[i]->state == TASK_RUNNING && task[i]->counter <= next->counter && task[i]->counter != 0) 
            next = task[i];
    }

    switch_to(next);
}
#endif

#ifdef PRIORITY
//DPRIORITY
void schedule(void) {
    /* YOUR CODE HERE */
    int i;
    struct task_struct* next;
    for(i = 1; i < NR_TASKS && task[i]!=NULL; i++){
        if(task[i]->counter > 0){
            next = task[i];
            break;
        }
    }
    if(i==NR_TASKS){
        for(i = 1; i < NR_TASKS && task[i]!=NULL; i++){
            task[i]->counter = rand();
            //printk("SET [PID = %d COUNTER = %d]\n", task[i]->pid, task[i]->counter);
        }
    }
    for(i = 1; i < NR_TASKS && task[i]!=NULL; i++)
    {
        if(task[i]->state == TASK_RUNNING && task[i]->priority >= next->priority && task[i]->counter!=0)
            next = task[i];
    }
    
    switch_to(next);
}
#endif


void do_timer(void) {
    /* 1. 如果当前线程是 idle 线程 或者 当前线程运行剩余时间为0 进行调度 */
    /* 2. 如果当前线程不是 idle 且 运行剩余时间不为0 则对当前线程的运行剩余时间减1 直接返回 */

    /* YOUR CODE HERE */
    if(current == idle || current->counter == 0)
        schedule();
    else if(current != idle && current->counter != 0)
        current -> counter --;
}

void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    //printk("switch to [PID = %d COUNTER = %d]\n", next->pid, next->counter);
    //if(current==idle)printk("idle process is running!\n");
    if(current != next){
        //printk("\nswitch to [PID = %d COUNTER = %d]\n", next->pid, next->counter);
        printk("[!] Switch from task %d[%lx] to task %d[%lx], prio: %d, counter: %d\n",current->pid,current,next->pid,next,next->priority,next->counter);

        //next->counter--;
         __switch_to(current, next);
    }
}

/*
* @mm          : current thread's mm_struct
* @address     : the va to look up
*
* @return      : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr){
    struct vm_area_struct *ptr=mm->mmap;
    while(ptr!=NULL){
        if(addr>=ptr->vm_start && addr<ptr->vm_end){
            break;
        }else {
            ptr=ptr->vm_next;
        }
    }
    return ptr;
}

/*
 * @mm     : current thread's mm_struct
 * @addr   : the suggested va to map
 * @length : memory size to map
 * @prot   : protection
 *
 * @return : start va
*/
uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot){
    struct vm_area_struct *new=(struct vm_area_struct*)kalloc(), *ptr=NULL;
    new->vm_mm=mm;
    long l=(long)length;
    while(l>0){
        ptr=find_vma(mm,addr);
        l-=PGSIZE;
    }
    if(ptr) addr=get_unmapped_area(mm,length);
    new->vm_start=addr;
    new->vm_end=addr+length;
    new->vm_prev=NULL;
    new->vm_next=mm->mmap;
    new->vm_flags=prot;
    if (mm->mmap==NULL){
        mm->mmap=new;
    }
    else{
        mm->mmap->vm_prev=new;
        mm->mmap=new;
    }
    return addr;
}

uint64 get_unmapped_area(struct mm_struct *mm, uint64 length){
    uint64 i=0,unmapped_area=0xFFFFFFFFFFFFFFFF,start;
    long l;
    struct vm_area_struct *ptr=NULL;
    while(unmapped_area!=0xFFFFFFFFFFFFFFFF){
        l=(long)length;
        ptr=NULL;
        start=0xFFFFFFFFFFFFFFFF;
        while(l>0){
            ptr=find_vma(mm,i);
            i+=PGSIZE;
            l-=PGSIZE;
            if(!ptr)break;
            if(start==0xFFFFFFFFFFFFFFFF) start=i-PGSIZE;
        }
        if(ptr) unmapped_area=start;
    }
    return unmapped_area;
}
/*
void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    idle = (struct task_struct*)kalloc();
    // 2. 设置 state 为 TASK_RUNNING;
    idle -> state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    idle -> counter = 0;
    idle -> priority = 0;
    // 4. 设置 idle 的 pid 为 0
    idle -> pid = 0;
    // 5. 将 current 和 taks[0] 指向 idle
    current = idle;
    task[0] = idle;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`, 
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址， `sp` 设置为 该线程申请的物理页的高地址

    int i,j;
    //int index1, index2, index3;
    //uint64*  putbl;
    //uint64*  pmtbl;
    for(int i = 1; i < 2; i++)
    {
        unsigned long ret;
        ret = kalloc();

        task[i] = ret;
        task[i] -> state = TASK_RUNNING;
        task[i] -> counter = 0;
        task[i] -> priority = rand();
        task[i] -> pid = i;

        task[i] -> thread.ra = &__dummy;
        task[i] -> thread.sp = ret + PGSIZE;
        task[i] -> thread_info=(struct thread_info *)kalloc();
        task[i] -> thread_info->kernel_sp = task[i] -> thread.sp - PA2VA_OFFSET;//PA
        task[i] -> thread_info->user_sp =(uint64)(kalloc() + PGSIZE - PA2VA_OFFSET);//PA
        task[i] -> thread.sscratch = USER_END;
        task[i] -> thread.sepc = USER_START;
        //task[i] -> thread.sepc=
        task[i] -> thread.sstatus = 0x40020;
        task[i] -> pgd = kalloc();
        for(j=0;j<512;j++)
        {
            task[i] -> pgd[j]=swapper_pg_dir[j];
        }
        task[i] -> mm=(struct mm_struct*)kalloc();
        //task[i] -> mm -> mmap =(struct vm_area_struct*)kalloc();
        do_mmap(task[i]->mm,USER_START,(uint64)(uapp_end)-(uint64)(uapp_start),6);
        do_mmap(task[i]->mm,USER_END-PGSIZE,PGSIZE,2);
        //struct pt_regs *trapframe
        //task[i]->trapframe=(struct pt_regs*)kalloc();
    }
    fs_test();
    printk("...proc_init done!\n");
}
*/
void elf_loader(struct task_struct *proc, const char *path){
    // read file (path) in cpio
    // load elf file to page table of task
    // set sepc to entry
    // set sstatus, sscratch ...   
    struct inode* ip = namei(path);
    struct elfhdr* elf=kalloc();
    struct proghdr* prog=kalloc();
    

    proc->pgd = kalloc();
    int i;
    for(i = 0; i < 512; i++) {
        proc->pgd[i] = swapper_pg_dir[i];
    }

    if(readi(ip, 0, elf, 0, sizeof(struct elfhdr)) != sizeof(struct elfhdr)) {
        printk("elf header loading error!\n");
        return -1;
    }
    if(elf->magic != ELF_MAGIC) {
        printk("elf magic number doesn't match!\n");
        return -1;
    }
    int ofs;
    for(i = 0, ofs = elf->phoff; i < elf->phnum; i++, ofs += sizeof(struct proghdr)) {
        if(readi(ip, 0, prog, ofs, sizeof(struct proghdr)) != sizeof(struct proghdr)) {
            printk("load program error!\n");
            return -1;
        } 
        if(prog->type != ELF_PROG_LOAD 
        || prog->memsz < prog->filesz
        || prog->vaddr + prog->memsz < prog->vaddr) {
            //printk("parse ph flags failed!\n");
            //return -1;
            continue;
        }
        // 这里kmalloc还没有实现
        void *segment=kalloc();
        create_mapping(proc->pgd, prog->vaddr, segment-PA2VA_OFFSET,prog->filesz , (uint64)(prog->flags +8)); // PTE_U 和 PTE_X
        if(prog->vaddr % PGSIZE != 0) {
            printk("prog vadder is not aligned!\n");
            return -1;
        }
        //readi(ip,0,segment,prog->off,prog->filesz);
        if(loadseg(proc->pgd, prog->vaddr, ip, prog->off, prog->filesz) == -1) {
            printk("loadseg failed!\n");
            return -1;
        }
    }

    proc -> thread_info->user_sp =(uint64)(kalloc() + PGSIZE - PA2VA_OFFSET);//PA
    //create_mapping(pgd, USER_END - PGSIZE, (uint64)kalloc(), PGSIZE, 15);
    create_mapping(proc->pgd,USER_END-PGSIZE,proc->thread_info->user_sp,PGSIZE,11);

    proc -> thread.sstatus = 0x40020;
    //csr_write(sstatus, sstatus_);

    proc -> thread.sscratch = (uint64)USER_END;
    //csr_write(sscratch, (uint64)USER_END);

    proc -> thread.sepc = elf->entry;
    //csr_write(sepc, elf->entry);

    //proc->pgd = pgd;
    //uint64 content = ((uint64)pgd-PA2VA_OFFSET)>>12;
    //content+=(uint64)0x8000000000000000;

    //csr_write(satp, content);
    // asm volatile("li t0, 0x8000000000000000");
    // asm volatile("la t1, proc->pgd");
    // asm volatile("li t2, 0xFFFFFFDF80000000");
    // asm volatile("sub t1, t1, t2");
    // asm volatile("srli t1,t1, 12");
    // asm volatile("add t0, t0, t1");
    // asm volatile("csrw satp, t0");
    // flush TLB
    //asm volatile("sfence.vma zero, zero");
    //readi(ip, 0, elf, 0, sizeof(struct elfhdr));
    //printk("entry=%d\n",elf->entry);


    //return 0;
    //loadseg(proc->pgd,USER_START,namei(path),0,PGSIZE);//?????????
    //proc->thread.sepc=USER_START;
    //proc->thread.sstatus=0x40020;
    //proc->thread.sscratch=USER_END;
}
void task_init(){
    /*
    allocate task struct
    elf_loader(task, "shell") // "shell" is the path of shell
    set task status to READY
    */
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    idle = (struct task_struct*)kalloc();
    // 2. 设置 state 为 TASK_RUNNING;
    idle -> state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    idle -> counter = 0;
    idle -> priority = 0;
    // 4. 设置 idle 的 pid 为 0
    idle -> pid = 0;
    // 5. 将 current 和 taks[0] 指向 idle
    current = idle;
    task[0] = idle;
    int i,j;
   
    task[1]=(struct task_struct*)kalloc();

    task[1]->state = TASK_RUNNING;
    task[1]->counter=1;
    task[1]->priority=rand();
    task[1]->pid=1;
    task[1]->thread.ra=&__dummy;
    task[1]->thread.sp=(uint64)(task[1])+PGSIZE;
    task[1] -> thread_info=(struct thread_info *)kalloc();
    task[1] -> thread_info->kernel_sp = task[1] -> thread.sp - PA2VA_OFFSET;//PA
    // task[1] -> thread_info->user_sp =(uint64)(kalloc() + PGSIZE - PA2VA_OFFSET);//PA
    //task[1] -> thread.sscratch = USER_END;
    //task[1] -> thread.sepc = USER_START;
    //task[1] -> thread.sstatus = 0x40020;
    // task[1] -> pgd = kalloc();
    // for(i=0;i<512;i++)
    // {
    //     task[1] -> pgd[i]=swapper_pg_dir[i];
    // }

    //create_mapping(task[1]->pgd,USER_START,kalloc()-PA2VA_OFFSET,PGSIZE,15);
    //create_mapping(task[1]->pgd,USER_END-PGSIZE,task[1]->thread_info->user_sp,PGSIZE,11);
    //proc_exec(task[1],"shell");
    elf_loader(task[1],"shell");
    //task[1] -> mm=(struct mm_struct*)kalloc();
    //do_mmap(task[1]->mm,USER_START,(uint64)(uapp_end)-(uint64)(uapp_start),7);
    //do_mmap(task[1]->mm,USER_END-PGSIZE,PGSIZE,3);

    //fs_test();
    printk("...proc_init done!\n");
}