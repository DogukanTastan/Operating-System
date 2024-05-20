
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>
#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;
using namespace myos::net;

char *itoa(int value, char *result, int base)
{
    // check that the base if valid
    if (base < 2 || base > 36)
    {
        *result = '\0';
        return result;
    }

    char *ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do
    {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while (value);

    //  negative sign
    if (tmp_value < 0)
        *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr)
    {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return result;
}

void printf(char *str)
{
    static uint16_t *VideoMemory = (uint16_t *)0xb8000;

    static uint8_t x = 0, y = 0;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        switch (str[i])
        {
        case '\n':
            x = 0;
            y++;
            break;
        default:
            VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
            x++;
            break;
        }

        if (x >= 80)
        {
            x = 0;
            y++;
        }

        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
                for (x = 0; x < 80; x++)
                    VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printNum(int num)
{
    char numberStr[10];
    itoa(num, numberStr, 10);
    printf(numberStr);
}

void printfHex(uint8_t key)
{
    char *foo = "00";
    char *hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char *foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;

public:
    MouseToConsole()
    {
        uint16_t *VideoMemory = (uint16_t *)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }

    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t *VideoMemory = (uint16_t *)0xb8000;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);

        x += xoffset;
        if (x >= 80)
            x = 79;
        if (x < 0)
            x = 0;
        y += yoffset;
        if (y >= 25)
            y = 24;
        if (y < 0)
            y = 0;

        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }
};

// long_running_program

common::uint32_t long_running_program(int n)
{
    common::uint32_t result = 0;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            result += i * j;
        }
    }
    return result;
}

void mysleep()
{
    for (int i = 0; i < 100000000; i++)
    {
        printf("");
    }
}

void collatz(int n)
{
    printNum(n);
    printf("-->");
    while (n != 1)
    {
        if (n % 2 == 0)
        {
            n = n / 2;
        }
        else
        {
            n = 3 * n + 1;
        }
        printNum(n);
        printf(" ");
    }
    printf(" \n");
}

int linear_search(int arr[], int n, int key)
{

    for (int i = 0; i < n; i++)
    {
        if (arr[i] == key)
        {
            return i;
        }
    }
    return -1;
}

void TaskLinearSearch()
{
    int arr[] = {10, 20, 80, 30, 60, 50, 110, 100, 130, 170};
    int n = sizeof(arr) / sizeof(arr[0]);
    int key = 80;
    int result = linear_search(arr, n, key);
    printf("Output: ");
    printNum(result);
    printf("\n");
    // mysleep();
    sys_exit();
}

int binary_search(int arr[], int left, int right, int key)
{
    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        // Check if key is present at mid
        if (arr[mid] == key)
        {
            return mid;
        }

        // If key is greater, ignore the left half
        if (arr[mid] < key)
        {
            left = mid + 1;
        }
        // If key is smaller, ignore the right half
        else
        {
            right = mid - 1;
        }
    }
    // If we reach here, then the element was not present
    return -1;
}

void TaskBinarySearch()
{
    int arr[] = {10, 20, 30, 50, 60, 80, 100, 110, 130, 170};
    int n = sizeof(arr) / sizeof(int);
    int key = 110;
    int result = binary_search(arr, 0, n - 1, key);
    printf("Output: ");
    printNum(result);
    printf("\n");
    // mysleep();
    sys_exit();
}

int32_t rand(int min, int max) // random func
{
    uint64_t counter;
    int32_t num;

    asm("rdtsc" : "=A"(counter));

    /* clock counter  */
    counter = counter * 1103515245 + 12345;
    num = (int)(counter / 65536) % (max - min);
    if (num < 0)
        num += max;
    return num + min;
}

void MicroKernel1()
{
    common::int32_t result;

    fork();
    int pid = getPid();

    if (pid == 0)
    {
        result = long_running_program(100);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }

    mysleep();

    fork();
    int pid2 = getPid();

    if (pid2 == 0)
    {
        result = long_running_program(150);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }

    mysleep();

    fork();
    int pid3 = getPid();

    if (pid3 == 0)
    {
        result = long_running_program(75);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }
    mysleep();

    fork();
    int pid4 = getPid();
    if (pid4 == 0)
    {
        result = long_running_program(85);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }
    mysleep();

    fork();
    int pid5 = getPid();
    if (pid5 == 0)
    {
        result = long_running_program(15);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }

    mysleep();
    fork();
    int pid6 = getPid();
    if (pid6 == 0)
    {
        result = long_running_program(122);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }
    mysleep();
    fork();
    int pid7 = getPid();
    if (pid7 == 0)
    {
        result = long_running_program(450);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }
    mysleep();
    fork();
    int pid8 = getPid();
    if (pid8 == 0)
    {
        result = long_running_program(185);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }
    mysleep();
    fork();
    int pid9 = getPid();
    if (pid9 == 0)
    {
        result = long_running_program(244);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }
    mysleep();
    fork();
    int pid10 = getPid();
    if (pid10 == 0)
    {
        result = long_running_program(59);
        printf("Result -->");
        printNum(result);
        printf("\n");
        sys_exit();
    }

    sys_exit();
    while (1)
        ;
}

void MicroKernel2()
{

    fork();
    int pid = getPid();

    if (pid == 0)
    {
        printf("TaskBinarySearch --> ");
        TaskBinarySearch();
        sys_exit();
    }

    mysleep();
    fork();
    int pid2 = getPid();

    if (pid2 == 0)
    {
        printf("TaskLinearSearch --> ");
        TaskLinearSearch();
        sys_exit();
    }

    mysleep();
    fork();
    int pid3 = getPid();

    if (pid3 == 0)
    {
        printf("TaskBinarySearch --> ");
        TaskBinarySearch();
        sys_exit();
    }

    mysleep();
    fork();
    int pid4 = getPid();

    if (pid4 == 0)
    {
        printf("TaskLinearSearch --> ");
        TaskLinearSearch();
        sys_exit();
    }

    mysleep();
    fork();
    int pid5 = getPid();

    if (pid5 == 0)
    {
        printf("TaskBinarySearch --> ");
        TaskBinarySearch();
        sys_exit();
    }

    mysleep();
    fork();
    int pid6 = getPid();

    if (pid6 == 0)
    {
        printf("TaskLinearSearch --> ");
        TaskLinearSearch();
        sys_exit();
    }

    sys_exit();
    while (1)
        ;
}

// Waitpid
/*
void taskA()
{
    fork();
    int pid = getPid();

    if (pid == 0)
    {
        printf("Child Task pid:");
        printfHex(pid);
        printf("  \n");
        for (int i = 0; i < 1000000000; i++)
        {
            printf("");
        }

        sys_exit();
    }
    else
    {
        fork();
    }

    printf("Task Ended...");
    printf("\n");

    while (1) ;
}
*/

void taskB()
{
    for (int i = 0; i < 100; i++)
    {
        printf("B\n");
    }
    sys_exit();
    while (1)
        ;
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(const void *multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello World! --- Dogukan TASTAN 1901042627\n");

    GlobalDescriptorTable gdt;

    uint32_t *memupper = (uint32_t *)(((size_t)multiboot_structure) + 8);
    size_t heap = 10 * 1024 * 1024;
    MemoryManager memoryManager(heap, (*memupper) * 1024 - heap - 10 * 1024);

    printf("heap: 0x");
    printfHex((heap >> 24) & 0xFF);
    printfHex((heap >> 16) & 0xFF);
    printfHex((heap >> 8) & 0xFF);
    printfHex((heap) & 0xFF);

    void *allocated = memoryManager.malloc(1024);
    printf("\nallocated: 0x");
    printfHex(((size_t)allocated >> 24) & 0xFF);
    printfHex(((size_t)allocated >> 16) & 0xFF);
    printfHex(((size_t)allocated >> 8) & 0xFF);
    printfHex(((size_t)allocated) & 0xFF);
    printf("\n");

    TaskManager taskManager(&gdt);

    Task task1(&gdt, MicroKernel2, 5);
    // Task task2(&gdt, taskB, 1);
    taskManager.AddTask(&task1);
    // taskManager.AddTask(&task2);

    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);
    interrupts.Activate();

    while (1)
        ;

    while (1)
    {
#ifdef GRAPHICSMODE
        desktop.Draw(&vga);
#endif
    }
}
