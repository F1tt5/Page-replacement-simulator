#pragma once
#include <string>
#include <assert.h>
#include <iostream>
#include <random>
#include "PageTable.h"
#include "sim.h"

#include <fstream>
#include <sstream>


typedef struct {
    int m_number; // номер страницы в ТС процесса
    int m_processID; //ID процесса страницы
    bool m_used;			// данная страница используется
    bool m_referenced;	    // бит обращения
    bool m_modified;		// бит изменения
    bool m_busy;			//бит выполнения страничной операции (загрузка/вытеснение); перераспределение страницы временно запрещено
    SimulatorTime m_referencedTime;
} MainMemoryPage;

class MainMemory
{
private:
    MainMemoryPage* m_pages;
    unsigned int m_npages; //Размер массива страниц

public:
    MainMemory(int enteredSize) { //Создание массива с заданным размером.
        m_npages = enteredSize;
        m_pages = new MainMemoryPage[m_npages];
        for (unsigned int i = 0; i < m_npages; ++i)
        {
            m_pages[i].m_number = -1;
            m_pages[i].m_processID = -1;
            m_pages[i].m_used = false;
            m_pages[i].m_referenced = false;
            m_pages[i].m_modified = false;
            m_pages[i].m_busy = false;
            m_pages[i].m_referencedTime = 0;
        }
        //SetName("Main Memory");
    }

    ~MainMemory() { // Деструктор массива.
        delete[] m_pages;
    }

    void Start() {};

    bool GetBitU(unsigned int position) { return m_pages[position].m_used; }
    bool GetBitR(unsigned int position) { return m_pages[position].m_referenced; }
    bool GetBitM(unsigned int position) { return m_pages[position].m_modified; }
    bool GetBitB(unsigned int position) { return m_pages[position].m_busy; }
    SimulatorTime GetRTime(unsigned int position) { return m_pages[position].m_referencedTime; }

    void SetBitU(unsigned int position, bool value) { m_pages[position].m_used = value; }
    void SetBitR(unsigned int position, bool value) { m_pages[position].m_referenced = value; }
    void SetBitM(unsigned int position, bool value) { m_pages[position].m_modified = value; }
    void SetBitB(unsigned int position, bool value) { m_pages[position].m_busy = value; }
    void SetRTime(unsigned int position, SimulatorTime time) { m_pages[position].m_referencedTime = time; }

    int GetVirtPage(unsigned int position) { return m_pages[position].m_number; }
    int GetProcessID(unsigned int position) { return m_pages[position].m_processID; }
    unsigned int GetFreeSpace();

    void SetPage(unsigned int position, unsigned int processID, unsigned int virtPage) {
        m_pages[position].m_number = virtPage;
        m_pages[position].m_processID = processID;
    }

    MainMemoryPage* GetMainMemory() { return m_pages; }
    unsigned int GetMainMemorySize() { return m_npages; }


    int PlacePage(); //Метод, находящий свободное место в ОП.
    bool CheckPage(unsigned int processID, unsigned int page); //Метод, проверяющий наличие страницы в ОП
    void DeleteProcess(unsigned int processID); //Метод, удаляющий страницы процесса из ОП.
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    int m_number;
    int m_processID;
    unsigned int m_returns;
}ArchiveMemoryPage;

class ArchiveMemory {
private:
    ArchiveMemoryPage* m_memory; //Массив страниц.
 

public:
    ArchiveMemory(int enteredSize) { //Создание массива с заданным размером.
        m_size = enteredSize;
        m_memory = new ArchiveMemoryPage[m_size];
        for (unsigned int i = 0; i < m_size; ++i)
        {
            m_memory[i].m_number = -1;
            m_memory[i].m_processID = -1;
            m_memory[i].m_returns = 0;
        }
        //SetName("Archive Memory");
    }

    ~ArchiveMemory() { // Деструктор массива.
        delete[] m_memory;
    }

    void Start() {};
    unsigned int GetFreeSpace();
    void SavePage(unsigned int processID, unsigned int virtPage); //Метод, добавляющий страницу в АС.
    bool CheckPage(unsigned int processID, unsigned int virtPage); //Метод проверки наличия страницы в АС
    void LoadPageFromArch(unsigned int processID, unsigned int page);
    unsigned int GetReturns(unsigned int processID, unsigned int virtPage); //Метод, дающий информацию о количестве возвращений страницы в ОП.
    void DeleteProcess(unsigned int processID);

    unsigned int m_size; //Размер массива.
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum OperationType
{
    OT_REG,
    OT_READ,
    OT_WRITE,
    OT_IO,
    OT_ZERO
};

class Process {
private:
    unsigned int m_processID; //ID процесса 
    float m_opreg; //Соотношение операций в регистрах ко всем операциям
    float m_opread; //Соотношение операций чтения ко всем операциям
    float m_opwrite; //Соотношение операций записи ко всем операциям
    float m_opinout; //Соотношение операций ввода/вывода ко всем операциям
    PageTable* m_table; //Указатель на таблицу преобразования
    SimulatorTime m_time; //Оставшееся время процессора
    unsigned int m_processSize;
    SimulatorTime m_reset = 300000;
    OperationType m_activeOperation;
    int m_activePage;



public:
    Process(unsigned int processID, unsigned int processSize, float opreg, float opread, float opwrite, float opinout, float* distribution) {
        //Задание ID процесса
        m_processID = processID;
        //Задание размера процесса (в страницах)
        m_processSize = processSize;
        //Задание соотношения операций процесса (следующие 4 строки)
        m_opreg = opreg;
        m_opread = opread;
        m_opwrite = opwrite;
        m_opinout = opinout;
        //Создание таблицы страниц
        PageTable* pageTable = new PageTable();
        pageTable->CreatePageTable(m_processSize);
        //Задание ссылки на таблицу процесса
        m_table = pageTable;
        m_distribution = new float[m_processSize];
        for (unsigned int i = 0; i < m_processSize; ++i) {
            m_distribution[i] = distribution[i];
        }
        for (unsigned int i = 0; i < 4; ++i) {
            m_operationsDone[i] = 0;
        }
        m_processPageUsageCount = new unsigned int[m_processSize];
        for (unsigned int i = 0; i < m_processSize; ++i) {
            m_processPageUsageCount[i] = 0;
        }
        m_processPageComeBacksCount = new unsigned int[m_processSize];
        for (unsigned int i = 0; i < m_processSize; ++i) {
            m_processPageComeBacksCount[i] = 0;
        }
        //Задание кванта времени
        m_time = m_reset;
        m_activeOperation = OT_ZERO;
        m_activePage = -1;
        //std::string str = "Process " + m_processID;
        //SetName(str);
    }
    ~Process() {
        delete m_table;
        m_table = nullptr;
        delete[] m_distribution;
        m_distribution = nullptr;
        delete[] m_processPageUsageCount;
        m_processPageUsageCount = nullptr;
        delete[] m_processPageComeBacksCount;
        m_processPageComeBacksCount = nullptr;
    }

    void Start() {};
    unsigned int GetProcessID() { return m_processID; }
    SimulatorTime GetTimeQuantum() { return m_time; }
    SimulatorTime GetReset() { return m_reset; }
    void DecreaseTimeQuantum(SimulatorTime time) { m_time -= time; }
    void ResetTimeQuantum() { m_time = m_reset; }
    unsigned int ChoosePage();
    OperationType ChooseOperation();
    PageTable* GetTable() { return m_table; }
    OperationType GetOperation() { return m_activeOperation; }
    int GetPage() { return m_activePage; }
    void SetOperation(OperationType activeOperation) { m_activeOperation = activeOperation; }
    void SetPage(unsigned int page) { m_activePage = page; }
    unsigned int GetSize() { return m_processSize; }
    void SetQuant(SimulatorTime quant) { m_reset = quant; m_time = m_reset; }

    long long m_operationsDone[4];
    unsigned int m_PageFaults = 0;
    unsigned int m_PageSaves = 0;
    unsigned int* m_processPageUsageCount;
    unsigned int* m_processPageComeBacksCount;
    float* m_distribution;
    SimulatorTime m_WorkTime = 0;
    SimulatorTime m_AllTime = 0;
    SimulatorTime m_TimeStartWaiting = 0;
    uint64_t m_nextEvent = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class Element
{
public:
    Element() { m_pNext = NULL; }
    virtual ~Element() {}

    Element* m_pNext = 0;
    Process* m_pProcess = 0;
    unsigned int m_pProcessID = 0;
    unsigned int m_realPage = 0;
    unsigned int m_virtPage = 0;
    unsigned int m_pOldProcessID = 0;
    unsigned int m_oldProcessPage = 0;
    unsigned int m_type = 0;


};

class Queue
{
public:
    Queue() {

    }
    ~Queue() {
        while (!IsEmpty()) {
            RemoveHead();
        }
    }

    bool IsEmpty() { return m_pFirstElement == NULL; }
    Element* GetHead() { return m_pFirstElement; }
    void RemoveHead()
    {
        if (m_pFirstElement)
        {
            Element* pRemovedElement = m_pFirstElement;
            m_pFirstElement = m_pFirstElement->m_pNext;
            delete pRemovedElement;
        }
    }

    unsigned int GetSize() {
        unsigned int count = 0;
        Element* pCurrentElement = m_pFirstElement;
        while (pCurrentElement != NULL)
        {
            count++;
            pCurrentElement = pCurrentElement->m_pNext;
        }
        return count;
    }

    void Add(unsigned int ProcessID)
    {
        Element* pElement = new Element;
        pElement->m_pProcessID = ProcessID;
        pElement->m_pNext = NULL;

        if (m_pFirstElement == NULL)
        {
            m_pFirstElement = pElement;
        }
        else
        {
            Element* pCurrentElement = m_pFirstElement;
            Element* pPrevElement = NULL;
            while (pCurrentElement != NULL)
            {
                pPrevElement = pCurrentElement;
                pCurrentElement = pCurrentElement->m_pNext;
            }
            if (pCurrentElement != NULL)
            {
                assert(pPrevElement);
                pPrevElement->m_pNext = pElement;
                pElement->m_pNext = pCurrentElement;
            }
            else
            {
                pPrevElement->m_pNext = pElement;
            }
        }
    }

    void AddForIO(unsigned int ProcessID, unsigned int OldProcessID, unsigned int OldProcessPage, unsigned int virtpage, unsigned int realpage, unsigned int type)
    {
        Element* pElement = new Element;
        pElement->m_pProcessID = ProcessID;
        pElement->m_pOldProcessID = OldProcessID;
        pElement->m_oldProcessPage= OldProcessPage;
        pElement->m_virtPage = virtpage;
        pElement->m_realPage = realpage;
        pElement->m_pNext = NULL;
        pElement->m_type = type;


        if (m_pFirstElement == NULL)
        {
            m_pFirstElement = pElement;
        }
        else
        {
            Element* pCurrentElement = m_pFirstElement;
            Element* pPrevElement = NULL;
            while (pCurrentElement != NULL)
            {
                pPrevElement = pCurrentElement;
                pCurrentElement = pCurrentElement->m_pNext;
            }
            if (pCurrentElement != NULL)
            {
                assert(pPrevElement);
                pPrevElement->m_pNext = pElement;
                pElement->m_pNext = pCurrentElement;
            }
            else
            {
                pPrevElement->m_pNext = pElement;
            }
        }
    }


    void AddForPageWaiting(unsigned int ProcessID, unsigned int type, unsigned int virtpage, unsigned int realpage)
    {
        Element* pElement = new Element;
        pElement->m_pProcessID = ProcessID;
        pElement->m_virtPage = virtpage;
        pElement->m_realPage = realpage;
        pElement->m_pNext = NULL;
        pElement->m_type = type;


        if (m_pFirstElement == NULL)
        {
            m_pFirstElement = pElement;
        }
        else
        {
            Element* pCurrentElement = m_pFirstElement;
            Element* pPrevElement = NULL;
            while (pCurrentElement != NULL)
            {
                pPrevElement = pCurrentElement;
                pCurrentElement = pCurrentElement->m_pNext;
            }
            if (pCurrentElement != NULL)
            {
                assert(pPrevElement);
                pPrevElement->m_pNext = pElement;
                pElement->m_pNext = pCurrentElement;
            }
            else
            {
                pPrevElement->m_pNext = pElement;
            }
        }
    }

    void Remove(unsigned int processID) {
        if (m_pFirstElement) {
            if (m_pFirstElement->m_pProcessID== processID) {
                Element* pRemovedElement = m_pFirstElement;
                m_pFirstElement = m_pFirstElement->m_pNext;
                delete pRemovedElement;
            }
            else {
                Element* pCurrentElement = m_pFirstElement->m_pNext;
                Element* pPrevElement = m_pFirstElement;
                while (pCurrentElement != NULL) {
                    if (pCurrentElement->m_pProcessID == processID) {
                        pPrevElement->m_pNext = pCurrentElement->m_pNext;
                        delete pCurrentElement;
                        return;
                    }
                    pPrevElement = pCurrentElement;
                    pCurrentElement = pCurrentElement->m_pNext;
                }
            }
        }
    }

private:
    Element* m_pFirstElement = NULL;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Processor
{
private:

    SimulatorTime m_timeForReg; //Время на операцию в регистре.
    SimulatorTime m_timeForRead; //Время на операцию в регистре.
    SimulatorTime m_timeForWrite; //Время на операцию в регистре.

public:

    Processor(SimulatorTime timeForReg, SimulatorTime timeForRead, SimulatorTime timeForWrite) {
        m_timeForReg = timeForReg;
        m_timeForRead = timeForRead;
        m_timeForWrite = timeForWrite;
       // SetName("Processor");
    }

    void Start() {};
    SimulatorTime GetTimeForRegOperation(); //Получить время выполнения операции в регистре.
    SimulatorTime GetTimeForReadOperation(); //Получить время выполнения операции чтения.
    SimulatorTime GetTimeForWriteOperation(); //Получить время выполнения операции записи.
    unsigned int UsePageTable(Process* process, unsigned int processID, unsigned int page); //Выполнение преобразования страницы.
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class IODevice : public Agent
{
private:

    SimulatorTime m_timeForOperation; //Время, с которым выполняется операция ввода вывода (за вычетом ожидания).

public:

    IODevice(SimulatorTime tFO) {
        m_timeForOperation = tFO;
        SetName("IODevice");
        m_InputOutput = new Queue();
    }
    ~IODevice() {
        delete m_InputOutput;
        m_InputOutput = nullptr;
    }
    void Start() {}

    SimulatorTime GetTimeForOperation() { return m_timeForOperation; }
    void StartIO();

    Queue* m_InputOutput = 0; //Ссылка на объект очереди процессов, ожидающих операции ввода/вывода
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class VirtualMemoryProcessor : public Agent
{
public:

    VirtualMemoryProcessor() {
        SetName("Virtual Memory Processor");
        m_PageWaiting = new Queue();
    }
    ~VirtualMemoryProcessor() {
        delete m_PageWaiting;
        m_PageWaiting = nullptr;
    }
    void Start() {}


    void StartPageQueue();
    void PageFault(unsigned int processID, unsigned int virtPage);
    void LoadPage(unsigned int processID, unsigned int virtPage, unsigned int realPage);
    void FinishPageLoad(unsigned int processID, unsigned int virtPage, unsigned int realPage);

    Queue* m_PageWaiting = 0; //Ссылка на объект очереди процессов, ожидающих страничной операции
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class StrategyElement
{
public:
    StrategyElement() { m_pNext = NULL; }
    virtual ~StrategyElement() {}

    StrategyElement* m_pNext = 0;
    unsigned int m_page = 0;
    int m_classID = -1;
    int m_classnumber = -1;
    unsigned char m_ageCounter = 0; // Счётчик возраста (8 бит)
};

class StrategyQueue
{
public:
    StrategyQueue() {

    }
    ~StrategyQueue() {
        while (!IsEmpty()) {
            RemoveHead();
        }
    }

    bool IsEmpty() { return m_pFirstElement == NULL; }
    StrategyElement* GetHead() { return m_pFirstElement; }
    void RemoveHead()
    {
        if (m_pFirstElement)
        {
            m_pFirstElement = m_pFirstElement->m_pNext;
        }
    }

    unsigned int GetSize() {
        unsigned int count = 0;
        StrategyElement* pCurrentElement = m_pFirstElement;
        while (pCurrentElement != NULL)
        {
            count++;
            pCurrentElement = pCurrentElement->m_pNext;
        }
        return count;
    }

    void Add(unsigned int page)
    {
        StrategyElement* pElement = new StrategyElement;
        pElement->m_page = page;
        pElement->m_pNext = NULL;

        if (m_pFirstElement == NULL) {
            m_pFirstElement = pElement;
        } else {
            StrategyElement* pCurrentElement = m_pFirstElement;
            StrategyElement* pPrevElement = NULL;
            while (pCurrentElement != NULL) {
                pPrevElement = pCurrentElement;
                pCurrentElement = pCurrentElement->m_pNext;
            }
            if (pCurrentElement != NULL)
            {
                assert(pPrevElement);
                pPrevElement->m_pNext = pElement;
                pElement->m_pNext = pCurrentElement;
            }
            else
            {
                pPrevElement->m_pNext = pElement;
            }
        }
    }

    void AddForClock(unsigned int page) {
        StrategyElement* pElement = new StrategyElement;
        pElement->m_page = page;

        if (!m_pFirstElement) {
            // Если очередь пуста
            m_pFirstElement = pElement;
            m_pFirstElement->m_pNext = m_pFirstElement; // Замыкаем очередь
            std::cout << "AddForClock: Added first page " << page << std::endl;
        }
        else {
            // Если очередь не пуста
            StrategyElement* pLastElement = m_pFirstElement;
            while (pLastElement->m_pNext != m_pFirstElement) {
                pLastElement = pLastElement->m_pNext;
            }
            pLastElement->m_pNext = pElement;
            pElement->m_pNext = m_pFirstElement; // Замыкаем очередь
            std::cout << "AddForClock: Added page " << page << std::endl;
        }
    }

    void RemoveHeadForClock() {
        if (!m_pFirstElement) {
            std::cout << "RemoveHeadForClock: Queue is empty" << std::endl;
            return;
        }

        if (m_pFirstElement->m_pNext == m_pFirstElement) {
            // Если в очереди только один элемент
            std::cout << "RemoveHeadForClock: Removing the only page " << m_pFirstElement->m_page << std::endl;
            delete m_pFirstElement;
            m_pFirstElement = nullptr;
        }
        else {
            // Если в очереди больше одного элемента
            StrategyElement* pLastElement = m_pFirstElement;
            while (pLastElement->m_pNext != m_pFirstElement) {
                pLastElement = pLastElement->m_pNext;
            }
            pLastElement->m_pNext = m_pFirstElement->m_pNext;
            std::cout << "RemoveHeadForClock: Removed page " << m_pFirstElement->m_page << std::endl;
            delete m_pFirstElement;
            m_pFirstElement = pLastElement->m_pNext;
        }
    }

    void RemoveForClock(unsigned int page) {
        if (!m_pFirstElement) {
            std::cout << "RemoveForClock: Queue is empty" << std::endl;
            return;
        }

        StrategyElement* pCurrent = m_pFirstElement;
        StrategyElement* pPrev = nullptr;

        // Ищем элемент для удаления
        do {
            if (pCurrent->m_page == page) {
                // Если элемент найден
                if (pCurrent == m_pFirstElement) {
                    if (m_pFirstElement->m_pNext == m_pFirstElement) {
                        // Если в очереди только один элемент
                        delete m_pFirstElement;
                        m_pFirstElement = nullptr;
                    }
                    else {
                        // Если в очереди больше одного элемента
                        StrategyElement* pLast = m_pFirstElement;
                        while (pLast->m_pNext != m_pFirstElement) {
                            pLast = pLast->m_pNext;
                        }
                        pLast->m_pNext = m_pFirstElement->m_pNext;
                        delete m_pFirstElement;
                        m_pFirstElement = pLast->m_pNext;
                    }
                }
                else {
                    // Если удаляемый элемент не голова
                    pPrev->m_pNext = pCurrent->m_pNext;
                    delete pCurrent;
                }
                std::cout << "RemoveForClock: Removed page " << page << std::endl;
                return;
            }
            pPrev = pCurrent;
            pCurrent = pCurrent->m_pNext;
        } while (pCurrent != m_pFirstElement);

        std::cout << "RemoveForClock: Page " << page << " not found" << std::endl;
    }
    
    void Remove(unsigned int page) {
        if (m_pFirstElement) {
            if (m_pFirstElement->m_page == page) {
                StrategyElement* pRemovedElement = m_pFirstElement;
                m_pFirstElement = m_pFirstElement->m_pNext;
                delete pRemovedElement;
            }
            else {
                StrategyElement* pCurrentElement = m_pFirstElement->m_pNext;
                StrategyElement* pPrevElement = m_pFirstElement;
                while (pCurrentElement != NULL) {
                    if (pCurrentElement->m_page == page) {
                        pPrevElement->m_pNext = pCurrentElement->m_pNext;
                        delete pCurrentElement;
                        return;
                    }
                    pPrevElement = pCurrentElement;
                    pCurrentElement = pCurrentElement->m_pNext;
                }
            }
        }
    }

    void PrintQueue() const {
        StrategyElement* pCurrentElement = m_pFirstElement;
        std::cout << "StrategyQueue: ";
        while (pCurrentElement != nullptr) {
            std::cout << pCurrentElement->m_page << " ";
            pCurrentElement = pCurrentElement->m_pNext;
        }
        std::cout << std::endl;
    }

private:
    StrategyElement* m_pFirstElement = NULL;
};

class PageReplacementStrategy
{
public:
    PageReplacementStrategy() {}
    virtual ~PageReplacementStrategy() {}

    virtual const char* GetName() = 0;
    virtual int GetPageToReplace() = 0;
    virtual void AddPage(unsigned int page) {}
    virtual void RemovePage(unsigned int page) {}
    virtual void SetMM(MainMemory* mainmemory) {}
    virtual SimulatorTime GetResetTime() = 0;
    virtual void SetResetTime(SimulatorTime resetTime) {}
    virtual StrategyQueue GetQueue() = 0;
    virtual void UpdateAgeCounters() {};
};

class FIFO : public PageReplacementStrategy
{
public:
    FIFO() {}
    virtual ~FIFO() {}

    const char* GetName() override
    {
        return "FIFO";
    }
    int GetPageToReplace() override
    {
        if (!m_pageQueue.IsEmpty()) {
            StrategyElement* pageIter = m_pageQueue.GetHead();
            while (true) {
                bool bitU = m_mainMemory->GetBitU(pageIter->m_page);
                bool bitB = m_mainMemory->GetBitB(pageIter->m_page);
                if ((!bitU) && (!bitB)) {
                    // Удаляем страницу из очереди и возвращаем ее
                    m_pageQueue.Remove(pageIter->m_page);
                    return pageIter->m_page;
                }
                else {
                    // Если бит U-used или B-busy равен true, пропускаем эту страницу и двигаемся дальше по очереди
                    pageIter = pageIter->m_pNext;
                    if (pageIter == nullptr) {
                        break;
                    }
                }
            }
            assert(0);
            return 666;
        }
        else {
            assert(0);
            return 666;
        }
    }

    void AddPage(unsigned int page) override
    {
        m_pageQueue.Add(page);
    }

    void RemovePage(unsigned int page) override
    {
        m_pageQueue.Remove(page);
    }

    void SetMM(MainMemory* mainmemory) {
        m_mainMemory = mainmemory;
    }

    SimulatorTime GetResetTime() override {
        return m_resetTime;
    }

    void SetResetTime(SimulatorTime resetTime) override {
        m_resetTime = resetTime;
    }

    StrategyQueue GetQueue() override {
        return m_pageQueue;
    }

private:
    StrategyQueue m_pageQueue;
    MainMemory* m_mainMemory = 0;
    SimulatorTime m_resetTime = 0;
};

class Random : public PageReplacementStrategy
{
public:
    Random() : m_gen(100) {}
    virtual ~Random() {}

    const char* GetName() override
    {
        return "Random";
    }

    int GetPageToReplace() override
    {
        if (!m_pageQueue.IsEmpty()) {
            // Получаем случайную страницу из памяти
            std::uniform_int_distribution<> dis(0, m_pageQueue.GetSize() - 1);
            int randomIndex = dis(m_gen);
            auto pageIter = m_pageQueue.GetHead();
            for (int i = 0; i < randomIndex; ++i) {
                pageIter = pageIter->m_pNext;
            }

            // Проверяем бит U страницы
            bool bitU = m_mainMemory->GetBitU(pageIter->m_page);
            bool bitB = m_mainMemory->GetBitB(pageIter->m_page);

            // Если биты B и U не установлены, удаляем страницу из очереди и возвращаем ее
            if ((!bitU) && (!bitB)) {
                m_pageQueue.Remove(pageIter->m_page);
                return pageIter->m_page;
            }
            // Если хотя бы один из битов B и U установлен, то выбираем другую страницу для замещения
            else {
                return GetPageToReplace();
            }
        }
        else {
            assert(0);
            return 666;
        }
    }

    void AddPage(unsigned int page) override
    {
        m_pageQueue.Add(page);
    }

    void RemovePage(unsigned int page) override
    {
        m_pageQueue.Remove(page);
    }

    void SetMM(MainMemory* mainmemory) {
        m_mainMemory = mainmemory;
    }

    SimulatorTime GetResetTime() override {
        return m_resetTime;
    }

    void SetResetTime(SimulatorTime resetTime) override {
        m_resetTime = resetTime;
    }

    StrategyQueue GetQueue() override {
        return m_pageQueue;
    }
private:
    StrategyQueue m_pageQueue;
    MainMemory* m_mainMemory  = 0;
    SimulatorTime m_resetTime = 0;
    std::mt19937 m_gen;
};

class SecondChance : public PageReplacementStrategy
{
public:
    SecondChance() {}
    virtual ~SecondChance() {}

    const char* GetName() override
    {
        return "Second chance";
    }

    int GetPageToReplace() override
    {
        if (!m_pageQueue.IsEmpty()) {
            while (true) {
                unsigned int page = m_pageQueue.GetHead()->m_page;
                bool bitU = m_mainMemory->GetBitU(page);
                bool bitB = m_mainMemory->GetBitB(page);
                bool bitR = m_mainMemory->GetBitR(page);
                if ((!bitU) && (!bitB)) {
                    if (bitR == true) {
                        m_mainMemory->SetBitR(page, false);
                    }
                    else if (bitR == false) {
                        m_pageQueue.Remove(page);
                        return page;
                    }
                }
                m_pageQueue.Add(page);
                m_pageQueue.RemoveHead();
            }
        }
        else {
            assert(0);
            return 666;
        }
    }

    void AddPage(unsigned int page) override
    {
        m_pageQueue.Add(page);
    }

    void RemovePage(unsigned int page) override
    {
        m_pageQueue.Remove(page);
    }

    void SetMM(MainMemory* mainmemory) {
        m_mainMemory = mainmemory;
    }

    SimulatorTime GetResetTime() override {
        return m_resetTime;
    }

    void SetResetTime(SimulatorTime resetTime) override {
        m_resetTime = resetTime;
    }

    StrategyQueue GetQueue() override {
        return m_pageQueue;
    }
private:
    StrategyQueue m_pageQueue;
    MainMemory* m_mainMemory = 0;
    SimulatorTime m_resetTime = 0;
};

class Clock : public PageReplacementStrategy
{
public:
    Clock() {}
    virtual ~Clock() {}

    const char* GetName() override
    {
        return "Clock";
    }

    int GetPageToReplace() override {
        if (!m_pageQueue.IsEmpty()) {
            unsigned int iterations = 0;
            unsigned int maxIterations = m_pageQueue.GetSize() * 2; // Максимум два прохода по очереди

            std::cout << "Clock::GetPageToReplace: Starting replacement process. Queue size: " << m_pageQueue.GetSize() << std::endl;

            while (iterations < maxIterations) {
                unsigned int page = m_pageQueue.GetHead()->m_page;
                bool bitU = m_mainMemory->GetBitU(page);
                bool bitB = m_mainMemory->GetBitB(page);
                bool bitR = m_mainMemory->GetBitR(page);

                std::cout << "Clock::GetPageToReplace: Checking page " << page
                    << " U: " << bitU << " B: " << bitB << " R: " << bitR << std::endl;

                if (!bitU && !bitB) {
                    if (bitR) {
                        std::cout << "Clock::GetPageToReplace: Resetting R bit for page " << page << std::endl;
                        m_mainMemory->SetBitR(page, false); // Сбрасываем бит R
                    }
                    else {
                        std::cout << "Clock::GetPageToReplace: Selected page " << page << " for replacement" << std::endl;
                        m_pageQueue.RemoveHeadForClock(); // Удаляем страницу из очереди
                        return page; // Возвращаем страницу для замещения
                    }
                }

                // Перемещаем страницу в конец очереди
                m_pageQueue.AddForClock(page);
                m_pageQueue.RemoveHeadForClock();

                iterations++;
                std::cout << "Clock::GetPageToReplace: Moved page " << page << " to the end of the queue. Iteration: " << iterations << std::endl;
            }

            // Если не удалось найти страницу за два прохода, выбираем первую страницу
            unsigned int page = m_pageQueue.GetHead()->m_page;
            std::cout << "Clock::GetPageToReplace: Fallback - selected page " << page << " for replacement" << std::endl;
            m_pageQueue.RemoveHeadForClock(); // Удаляем страницу из очереди
            return page; // Возвращаем страницу для замещения
        }
        else {
            std::cout << "Clock::GetPageToReplace: Queue is empty!" << std::endl;
            assert(0); // Очередь не должна быть пустой
            return -1; // Возвращаем -1 в случае ошибки
        }
    }

    void AddPage(unsigned int page) override
    {
        m_pageQueue.AddForClock(page);
    }

    void RemovePage(unsigned int page) override
    {
        m_pageQueue.RemoveForClock(page);
    }

    void SetMM(MainMemory* mainmemory) {
        m_mainMemory = mainmemory;
    }

    SimulatorTime GetResetTime() override {
        return m_resetTime;
    }

    void SetResetTime(SimulatorTime resetTime) override {
        m_resetTime = resetTime;
    }

    StrategyQueue GetQueue() override {
        return m_pageQueue;
    }
private:
    StrategyQueue m_pageQueue;
    MainMemory* m_mainMemory = 0;
    SimulatorTime m_resetTime = 0;
};

class NRU : public PageReplacementStrategy
{
public:
    NRU() : m_gen(100) {}
    virtual ~NRU() {}

    const char* GetName() override
    {
        return "NRU";
    }

    int GetPageToReplace() override
    {
        if (!m_pageQueue.IsEmpty()) {
            unsigned int class0Size = 0;
            unsigned int class1Size = 0;
            unsigned int class2Size = 0;
            unsigned int class3Size = 0;

            for (auto pageIter = m_pageQueue.GetHead(); pageIter != nullptr; pageIter = pageIter->m_pNext) {
                unsigned int page = pageIter->m_page;
                bool bitU = m_mainMemory->GetBitU(page);
                bool bitB = m_mainMemory->GetBitB(page);
                bool bitR = m_mainMemory->GetBitR(page);
                bool bitM = m_mainMemory->GetBitM(page);
                if ((!bitU) && (!bitB)) {
                    if ((bitR) && (bitM)) {
                        class3Size += 1;
                        pageIter->m_classID = 3;
                        pageIter->m_classnumber = class3Size;
                    }
                    else if ((bitR) && (!bitM)) {
                        class2Size += 1;
                        pageIter->m_classID = 2;
                        pageIter->m_classnumber = class2Size;
                    }
                    else if ((!bitR) && (bitM)) {
                        class1Size += 1;
                        pageIter->m_classID = 1;
                        pageIter->m_classnumber = class1Size;
                    }
                    else if ((!bitR) && (!bitM)) {
                        class0Size += 1;
                        pageIter->m_classID = 0;
                        pageIter->m_classnumber = class0Size;
                    }
                }
            }
            int selectedClass = -1;
            int selectedClassSize = -1;

            if (class3Size != 0) {
                selectedClass = 3;
                selectedClassSize = class3Size;
            }
            if (class2Size != 0) {
                selectedClass = 2;
                selectedClassSize = class2Size;
            }
            if (class1Size != 0) {
                selectedClass = 1;
                selectedClassSize = class1Size;
            }
            if (class0Size != 0) {
                selectedClass = 0;
                selectedClassSize = class0Size;
            }

            // Получаем случайную страницу из памяти
            std::uniform_int_distribution<> dis(1, selectedClassSize);
            int randomIndex = dis(m_gen);
            unsigned int resultpage = -1;
            for (auto pageIter = m_pageQueue.GetHead(); pageIter != nullptr; pageIter = pageIter->m_pNext) {
                if ((pageIter->m_classID == selectedClass) && (pageIter->m_classnumber == randomIndex)) {
                    resultpage = pageIter->m_page;
                }
            }
            for (auto pageIter = m_pageQueue.GetHead(); pageIter != nullptr; pageIter = pageIter->m_pNext) {
                pageIter->m_classID = -1;
                pageIter->m_classnumber = -1;
            }
            m_pageQueue.Remove(resultpage);
            return resultpage;
        }
        else {
            assert(0);
            return 666;
        }
    }

    void AddPage(unsigned int page) override
    {
        m_pageQueue.Add(page);
    }

    void RemovePage(unsigned int page) override
    {
        m_pageQueue.Remove(page);
    }

    void SetMM(MainMemory* mainmemory) {
        m_mainMemory = mainmemory;
    }

    SimulatorTime GetResetTime() override {
        return m_resetTime;
    }

    void SetResetTime(SimulatorTime resetTime) override {
        m_resetTime = resetTime;
    }

    StrategyQueue GetQueue() override {
        return m_pageQueue;
    }
private:
    StrategyQueue m_pageQueue;
    MainMemory* m_mainMemory = 0;
    SimulatorTime m_resetTime = 0;
    std::mt19937 m_gen;
};

class Aging : public PageReplacementStrategy
{
public:
    Aging() {}
    virtual ~Aging() {}

    const char* GetName() override
    {
        return "Aging";
    }


    int GetPageToReplace() override {
        if (m_pageQueue.IsEmpty()) {
            std::cerr << "Aging: Queue is empty!" << std::endl;
            return -1;
        }

        StrategyElement* pCurrent = m_pageQueue.GetHead();
        StrategyElement* pSelected = nullptr;
        unsigned char minAge = 0xFF;

        // Ищем страницу с минимальным счётчиком, у которой BitU=false и BitB=false
        while (pCurrent != nullptr) {
            bool isUsed = m_mainMemory->GetBitU(pCurrent->m_page);
            bool isBusy = m_mainMemory->GetBitB(pCurrent->m_page);

            if (!isUsed && !isBusy && pCurrent->m_ageCounter < minAge) {
                minAge = pCurrent->m_ageCounter;
                pSelected = pCurrent;
            }
            pCurrent = pCurrent->m_pNext;
        }

        if (pSelected == nullptr) {
            std::cerr << "Aging: No suitable page found (all pages are used or busy)!" << std::endl;
            return -1; // Нет страниц для замещения
        }

        int pageToReplace = pSelected->m_page;
        m_pageQueue.Remove(pageToReplace); // Удаляем только если страница найдена
        return pageToReplace;
    }

    void AddPage(unsigned int page) override
    {
        m_pageQueue.Add(page);
    }

    void RemovePage(unsigned int page) override
    {
        m_pageQueue.Remove(page);
    }

    void UpdateAgeCounters() {
        StrategyElement* pCurrentElement = m_pageQueue.GetHead();
        while (pCurrentElement != nullptr) {
            pCurrentElement->m_ageCounter >>= 1;
            if (m_mainMemory->GetBitR(pCurrentElement->m_page)) {
                pCurrentElement->m_ageCounter |= 0x80;
                m_mainMemory->SetBitR(pCurrentElement->m_page, false);
            }
            pCurrentElement = pCurrentElement->m_pNext;
        }
    }

    void SetMM(MainMemory* mainmemory) override {
        m_mainMemory = mainmemory;
    }

    SimulatorTime GetResetTime() override {
        return m_resetTime;
    }

    void SetResetTime(SimulatorTime resetTime) override {
        m_resetTime = resetTime;
    }

    StrategyQueue GetQueue() override {
        return m_pageQueue;
    }

private:
    StrategyQueue m_pageQueue;
    MainMemory* m_mainMemory = nullptr;
    SimulatorTime m_resetTime = 0;
};

class LRU : public PageReplacementStrategy
{
public:
    LRU() {}
    virtual ~LRU() {}

    const char* GetName() override
    {
        return "LRU";
    }

    int GetPageToReplace() override
    {
        if (!m_pageQueue.IsEmpty()) {
            unsigned int resultPage = 0;
            SimulatorTime minTime = 0;
            for (auto pageIter = m_pageQueue.GetHead(); pageIter != nullptr; pageIter = pageIter->m_pNext) {
                unsigned int page = pageIter->m_page;
                bool bitU = m_mainMemory->GetBitU(page);
                bool bitB = m_mainMemory->GetBitB(page);
                SimulatorTime time = m_mainMemory->GetRTime(page);
                if ((!bitU) && (!bitB)) {
                    if ((minTime == 0) || (time < minTime)) {
                        minTime = time;
                        resultPage = page;
                    }
                }
            }
            if (minTime) {
                m_pageQueue.Remove(resultPage);
                return resultPage;
            }
            else {
                assert(0);
                return 666;
            }
        }
        else {
            assert(0);
            return 666;
        }
    }

    void AddPage(unsigned int page) override
    {
        m_pageQueue.Add(page);
    }

    void RemovePage(unsigned int page) override
    {
        m_pageQueue.Remove(page);
    }

    void SetMM(MainMemory* mainmemory) {
        m_mainMemory = mainmemory;
    }

    SimulatorTime GetResetTime() override {
        return m_resetTime;
    }

    void SetResetTime(SimulatorTime resetTime) override {
        m_resetTime = resetTime;
    }

    StrategyQueue GetQueue() override {
        return m_pageQueue;
    }
private:
    StrategyQueue m_pageQueue;
    MainMemory* m_mainMemory = 0;
    SimulatorTime m_resetTime = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OperatingSystem : public Agent
{
private:
    unsigned int m_pageSize = 0; //Размер страниц в системе (в байтах)
    MainMemory* m_mainMemory = 0; //Ссылка на объект ОП
    Processor* m_processor = 0; //Ссылка на объект процессора
    IODevice* m_iodevice = 0; //Ссылка на объект устройства ввода/вывода
    PageReplacementStrategy* m_Strategy = 0; //Ссылка на объект стратегии замещения страниц
    VirtualMemoryProcessor* m_VMP = 0; //Ссылка на объект устройства управления виртуальной памятью

    typedef struct {
        SimulatorTime m_time;
        float m_OverheadExpenses;
        unsigned int m_OPspace;
        unsigned int m_AMspace;
    } Info;
    SimulatorTime m_AgingUpdateInterval;

public:

    static OperatingSystem* g_pOperatingSystem;

    OperatingSystem() {
        m_Active = new Queue();
        m_AgingUpdateInterval = 100;
        SetName("Operating System");
    }

    ~OperatingSystem() {
        delete m_Strategy;
        m_Strategy = nullptr;

        delete m_mainMemory;
        m_mainMemory = nullptr;

        delete m_archiveMemory;
        m_archiveMemory = nullptr;

        delete m_processor;
        m_processor = nullptr;

        delete m_iodevice;
        m_iodevice = nullptr;

        delete m_VMP;
        m_VMP = nullptr;

        // Очищаем активную очередь
        if (m_Active) {
            while (!m_Active->IsEmpty()) {
                m_Active->RemoveHead();
            }
            delete m_Active;
            m_Active = nullptr;
        }

        // Очищаем процессы
        if (m_processes) {
            for (unsigned int i = 0; i < m_processesSize; i++) {
                if (m_processes[i]) {
                    delete m_processes[i];
                    m_processes[i] = nullptr;
                }
            }
            delete[] m_processes;
            m_processes = nullptr;
        }

        // Очищаем таблицу информации
        if (m_InfoTable) {
            delete[] m_InfoTable;
            m_InfoTable = nullptr;
        }
    }


    void Start();

    void ChooseStrategy(unsigned int strategyNumber);
    float* ChooseDistribution(unsigned int processPageSize, unsigned int distributionID, float parameter);
    void StartProcess(unsigned int processSystemID);
    void RemoveProcess(unsigned int processID);
    void ResetBitR();
    void UpdateAgeCounters();
    void CollectInfo(unsigned int iter, SimulatorTime time, unsigned int number);

    void StartDispatch();
    void FinishRegDispatch(SimulatorTime time, unsigned int processID);
    void FinishReadDispatch(unsigned int processID, unsigned int realPage);
    void FinishWriteDispatch(unsigned int processID, unsigned int realPage);
    void FinishIODispatch(unsigned int processID);

    void SetMainMemory(MainMemory* mainMemory) { m_mainMemory = mainMemory; }
    void SetArchiveMemory(ArchiveMemory* archiveMemory) { m_archiveMemory = archiveMemory; }
    void SetProcessor(Processor* processor) { m_processor = processor; }
    void SetIODevice(IODevice* iodevice) { m_iodevice = iodevice; }
    void SetVMP(VirtualMemoryProcessor* VMP) { m_VMP = VMP; }
    void SetInfoSize(unsigned int size) { m_InfoTable = new Info[size]; }

    unsigned int GetPageSize() { return m_pageSize; }
    Process* GetProcess(unsigned int processID);

    MainMemory* GetMainMemory() { return m_mainMemory; }
    IODevice* GetIODevice() { return m_iodevice; }
    PageReplacementStrategy* GetStrategy() { return m_Strategy; }
    VirtualMemoryProcessor* GetVMP() { return m_VMP; }

    SimulatorTime GetAgingTime() { return m_AgingUpdateInterval; }

    Process** m_processes = 0; //Список активных процессов
    unsigned int m_processesSize = 0;
    Queue* m_Active = 0; //Ссылка на объект активной очереди процессов
    unsigned int m_Scale = 1000;

    unsigned int m_PageFaults = 0;
    unsigned int m_PageSaves = 0;
    unsigned int m_output = 0;
    Info* m_InfoTable = 0;

    SimulatorTime m_quant = 0;
    ArchiveMemory* m_archiveMemory = 0; //Ссылка на объект АС
    //Время подготовки к выполнению диспетчеризации
    SimulatorTime m_DispatchTime = 10000;
    //Время подготовки к выполнению операции ввода/вывода
    SimulatorTime m_IOTime = 10000;
    //Время подготовки к выполнению операции по управлению виртуальной памятью
    SimulatorTime m_PageActionTime = 10000;
};


