#pragma once
typedef struct {
    int m_realPage; // Номер страницы в ОП
    bool m_presence; // Бит присутствия страницы в ОП
} PageTableStr;

class PageTable
{
private:
    PageTableStr* m_pages = 0;
    unsigned int m_npages = 0; // Количество строк в таблице.
public:
    PageTable() {

    }

    ~PageTable() {
        delete[] m_pages;
    }

    void CreatePageTable(unsigned int enteredSize) {
        m_npages = enteredSize;
        m_pages = new PageTableStr[m_npages];
        for (unsigned int i = 0; i < m_npages; ++i)
        {
            m_pages[i].m_realPage = -1;
            m_pages[i].m_presence = false;
        }
    }

    unsigned int GetRealPage(unsigned int virtualPage) { return m_pages[virtualPage].m_realPage; }
    void AddRealPage(unsigned int virtualPage, unsigned int realPageNumber) { m_pages[virtualPage].m_realPage = realPageNumber; }
    void SetPresence(unsigned int virtualPage, unsigned int bit) { m_pages[virtualPage].m_presence = bit; }
    bool GetPresence(unsigned int virtualPage) { return m_pages[virtualPage].m_presence; }
};
