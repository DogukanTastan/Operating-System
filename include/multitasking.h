
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos
{
    namespace hardwarecommunication
    {
        class InterruptHandler;
    }

    struct CPUState
    {
        common::uint32_t eax; //a
        common::uint32_t ebx; //b
        common::uint32_t ecx; // return value
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        common::uint32_t eip; // instruction pointer
        common::uint32_t cs;  // code segment
        common::uint32_t eflags;
        common::uint32_t esp;  // stack pointer
        common::uint32_t ss;
    } __attribute__((packed));

    enum TaskState
    {
        WAITING,
        READY,
        FINISHED
    };

    class Task
    {
        friend class TaskManager;

    private:
        static common::uint32_t pIdCounter;
        common::uint8_t stack[4096]; // 4 KiB
        common::uint32_t pId = 0;
        common::uint32_t pPid = 0;
        common::uint32_t priority = 1;
        TaskState taskState;
        common::uint32_t waitPid;
        CPUState *cpustate;

    public:
        Task(GlobalDescriptorTable *gdt, void entrypoint());
        Task(GlobalDescriptorTable *gdt, void entrypoint(),common::uint32_t get_priority);
        Task();
        bool isChild=false;
        common::uint32_t getId();
        ~Task();
    };

    class TaskManager
    {
        friend class hardwarecommunication::InterruptHandler;

    private:
        Task tasks[256];
        int numTasks;
        int currentTask;

        GlobalDescriptorTable *gdt = nullptr;
        int getIndex(common::uint32_t pid);

    public:
        void PrintProcessTable();
        //common::uint32_t AddTask(void entrypoint()); // for syscall operations
        common::uint32_t ExecTask(void entrypoint());
        common::uint32_t GetPId();
        common::uint32_t ForkTask(CPUState *cpustate);
        bool ExitCurrentTask();
        bool WaitTask(common::uint32_t pid);
       
        TaskManager(GlobalDescriptorTable *gdtImport);
        TaskManager();
        ~TaskManager();
        bool AddTask(Task *task);
        CPUState* Schedule(CPUState* cpustate);
    };

}

#endif