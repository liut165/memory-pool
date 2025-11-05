#include <iostream> 
#include <string>
#include <cstring>
#include <stdlib.h>
#include <vector>
using namespace std;

//设置内存对齐值——
static inline size_t align_up(size_t n, size_t align)
{
    return (n+(align -1) & ~(align - 1));
}
/*内存池大小——page：——> 切分 ——> [块1] [块2]...[块n]*/
class FixedMemoryPool{
public:
    FixedMemoryPool(size_t block_size, size_t block_nums= 4096)
    {
        m_block_nums = block_nums;
        m_block_per_size = adjust_block(block_size);
        free_ptr = nullptr;
    }
    ~FixedMemoryPool()
    {
        for (void *p : pages)
            ::operator delete[](p);
    }
    void* allocate()
    {
        if (!free_ptr)
            expand();
        Node *head = free_ptr;
        free_ptr = head->next;
        return head;
    }
    void deallocate(void *p)
    {
        if (!p)return;
        //头插法加入链表
        Node *n = static_cast<Node*>(p);
        n->next = free_ptr;
        free_ptr = n;   
    }
    size_t get_block_size(){return m_block_per_size;}
    size_t get_block_nums(){return m_block_nums;}
private:
    //  调整每块的大小
    size_t adjust_block(size_t s)
    {
        size_t min = sizeof(void*);
        size_t a = align_up(s < min ? min : s, alignof(void*));//保证内存块大小为内存对齐值
        return a;
    }
    // 向系统申请页内存
    void expand()
    {
        size_t page_bytes = m_block_nums * m_block_per_size;
        char *page = static_cast<char *>(::operator new[](page_bytes));
        pages.push_back(page);

         //拆分页为小块——把所有小块串成链表
        for (size_t i =0; i < m_block_nums; i++)
        {
            char *block_addr = page + i * m_block_per_size;
            Node *n = reinterpret_cast<Node *>(block_addr);
            n->next = free_ptr;
            free_ptr = n;
        }
    }
    //将空快链接成一个链表，以便使用
    struct Node{Node *next;};
    Node *free_ptr; //指向链表的头结点
    size_t m_block_per_size;
    size_t m_block_nums;
    vector<void*> pages;
};

/*===================实例应用=====================*/
struct g_demo
{
    float a, b, c;
    int life;
    static void* operator new(size_t n);
    static void operator delete(void*p) noexcept;
    void update(){life++;}
};
//建立内存池对象
static FixedMemoryPool demo_pool(sizeof(g_demo), 1024);
//重载new和delete，每次从内存池中分配内存释放
void* g_demo::operator new(size_t n)
{
    return demo_pool.allocate();
}
void g_demo::operator delete(void*p) noexcept
{
    demo_pool.deallocate(p);
}

int main()
{
    vector<g_demo*> test;//存储指向所分配的内存块指针的容器
    test.reserve(10000);
    for (size_t i =0; i< 10000; i++)
    {
        g_demo *p = new g_demo{0.0, 0.0, 0.0, 1};
        test.push_back(p);
    }
    //根据指针释放内存
    for (auto *p : test)
        delete p;

    cout <<"block_per_size= "<<demo_pool.get_block_size()
         <<" , block_nums = "<<demo_pool.get_block_nums()<<endl;
}
