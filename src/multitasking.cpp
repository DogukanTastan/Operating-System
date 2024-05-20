
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

myos::common::uint32_t myos::Task::pIdCounter = 0;
enum SYSCALLS
{
    EXIT,
    GETPID,
    WAITPID,
    FORK,
    EXEC,
    PRINTF
};

Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{
    cpustate = (CPUState *)(stack + 4096 - sizeof(CPUState));

    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;

    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = 0;

    cpustate->eip = (uint32_t)entrypoint;
    cpustate->cs = gdt->CodeSegmentSelector();
    cpustate->eflags = 0x202;
}

Task::Task(GlobalDescriptorTable *gdt, void entrypoint(), common::uint32_t get_priority)
{
    cpustate = (CPUState *)(stack + 4096 - sizeof(CPUState));
    priority = get_priority;

    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;

    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = 0;

    cpustate->eip = (uint32_t)entrypoint;
    cpustate->cs = gdt->CodeSegmentSelector();
    cpustate->eflags = 0x202;
}

void printf(char *str);
void printNum(int num);
char *itoa(int value, char *result, int base);


void TaskManager::PrintProcessTable()
{
    printf("\n***********************************\n");
    printf("PID    PID    STATE\n");
    for (int i = 0; i < numTasks; i++)
    {
        printNum(tasks[i].pId);
        printf("       ");
        printNum(tasks[i].pId);
        printf("     ");
        if (tasks[i].taskState == TaskState::READY)
        {
            if (i == currentTask)
                printf("RUNNING");
            else
                printf("READY");
        }
        else if (tasks[i].taskState == TaskState::WAITING)
            printf("WAITING");
        else if (tasks[i].taskState == TaskState::FINISHED)
            printf("FINISHED");

        printf("\n");
    }

    printf("\n******************************\n");
    for (int i = 0; i < 1000000000; i++)
        printf("");
}

Task::Task()
{
}

Task::~Task()
{
}

TaskManager::TaskManager(GlobalDescriptorTable *gdtImport)
{
    gdt = gdtImport;
    numTasks = 0;
    currentTask = -1;
}

TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task *task)
{
    if (numTasks > 256)
        return false;
    tasks[numTasks].taskState = READY;
    tasks[numTasks].pId = ++Task::pIdCounter;
    tasks[numTasks].cpustate = (CPUState *)(tasks[numTasks].stack + 4096 - sizeof(CPUState));

    tasks[numTasks].cpustate->eax = task->cpustate->eax;
    tasks[numTasks].cpustate->ebx = task->cpustate->ebx;
    tasks[numTasks].cpustate->ecx = task->cpustate->ecx;
    tasks[numTasks].cpustate->edx = task->cpustate->edx;
    tasks[numTasks].priority = task->priority;

    tasks[numTasks].cpustate->esi = task->cpustate->esi;
    tasks[numTasks].cpustate->edi = task->cpustate->edi;
    tasks[numTasks].cpustate->ebp = task->cpustate->ebp;

    tasks[numTasks].cpustate->eip = task->cpustate->eip;
    tasks[numTasks].cpustate->cs = task->cpustate->cs;

    tasks[numTasks].cpustate->eflags = task->cpustate->eflags;

    numTasks++;
    return true;
}

void printfHex(uint8_t key);
void printf(char *str);

common::uint32_t TaskManager::GetPId()
{

    if (tasks[currentTask].isChild)
    {

        return 0;
    }
    else
    {
        return tasks[currentTask].pId;
    }
}

bool TaskManager::ExitCurrentTask()
{
    tasks[currentTask].taskState = FINISHED;
    PrintProcessTable();
    return true;
}

int TaskManager::getIndex(common::uint32_t pid)
{
    int index = -1;

    printf("numTasks :");
    printNum(numTasks);
    printf(" \n");

    for (int i = 0; i < numTasks; i++)
    {
        /* printf("currentTask pid :");
         printNum(tasks[i].pId);
         printf(" \n");*/

        if (tasks[i].pId == pid)
        {
            index = i;
            break;
        }
    }
    return index;
}

common::uint32_t TaskManager::ExecTask(void entrypoint())
{

    tasks[currentTask].taskState = READY;
    tasks[currentTask].cpustate = (CPUState *)(tasks[currentTask].stack + 4096 - sizeof(CPUState));

    tasks[currentTask].cpustate->eax = 0;
    tasks[currentTask].cpustate->ebx = 0;
    tasks[currentTask].cpustate->ecx = tasks[currentTask].pId;
    tasks[currentTask].cpustate->edx = 0;

    tasks[currentTask].cpustate->esi = 0;
    tasks[currentTask].cpustate->edi = 0;
    tasks[currentTask].cpustate->ebp = 0;

    tasks[currentTask].cpustate->eip = (uint32_t)entrypoint;

    tasks[currentTask].cpustate->cs = gdt->CodeSegmentSelector();
    tasks[currentTask].cpustate->eflags = 0x202;

    return (uint32_t)tasks[currentTask].cpustate;
}

common::uint32_t TaskManager::ForkTask(CPUState *cpustate)
{
    tasks[numTasks].isChild = true;
    tasks[numTasks].priority = 5;

    if (numTasks >= 256)
        return -1;

    tasks[numTasks].taskState = READY;
    tasks[numTasks].pPid = tasks[currentTask].pId;
    tasks[numTasks].pId = ++Task::pIdCounter;
    tasks[numTasks].priority = 5;

    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++)
    {
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];
    }

    common::uint32_t currentTaskOffset = (((common::uint32_t)cpustate) - ((common::uint32_t)tasks[currentTask].stack));
    tasks[numTasks].cpustate = (CPUState *)(tasks[numTasks].stack + currentTaskOffset);

    tasks[numTasks].cpustate->ecx = 0;
    numTasks++;

    return tasks[numTasks - 1].pId;
}

bool TaskManager::WaitTask(common::uint32_t esp)
{
    CPUState *cpustate = (CPUState *)esp;
    common::uint32_t pid = cpustate->ebx;

    printf("waitTask a gelen gelen beklenecek pid:");
    printNum(pid);
    printf("\n");

    tasks[currentTask].cpustate = cpustate;
    tasks[currentTask].waitPid = pid;
    tasks[currentTask].taskState = WAITING;

    printf("beklenecek pid  :");
    printNum(tasks[currentTask].waitPid);
    printf("\n");

    return true;
}

CPUState *TaskManager::Schedule(CPUState *cpustate)
{
    if (numTasks <= 0)
        return cpustate;

    if (currentTask >= 0)
    {
        tasks[currentTask].cpustate = cpustate;
    }

    // Başlangıçta en yüksek öncelikli görevi ve onun indeksini bul
    int highestPriorityTask = -1;
    common::uint32_t highestPriority = 0;

    // Mevcut görevden başlayarak tüm görevleri kontrol et
    int findTask = (currentTask + 1) % numTasks;
    int startTask = findTask;
    do
    {
        if (tasks[findTask].taskState == READY)
        {
            if (highestPriorityTask == -1 || tasks[findTask].priority > highestPriority)
            {
                highestPriority = tasks[findTask].priority;
                highestPriorityTask = findTask;
            }
        }
        else if (tasks[findTask].taskState == WAITING && tasks[findTask].waitPid >= 0)
        {
            int waitTaskIndex = (tasks[findTask].waitPid) % numTasks;
            if (tasks[waitTaskIndex].taskState == FINISHED)
            {
                tasks[findTask].taskState = READY;
                tasks[findTask].waitPid = -1;
                if (highestPriorityTask == -1 || tasks[findTask].priority > highestPriority)
                {
                    highestPriority = tasks[findTask].priority;
                    highestPriorityTask = findTask;
                }
            }
        }
        findTask = (findTask + 1) % numTasks;
    } while (findTask != startTask);

    // En yüksek öncelikli görevi güncel görev olarak belirle
    if (highestPriorityTask != -1)
    {
        currentTask = highestPriorityTask;
    }

    return tasks[currentTask].cpustate;
}
