#include "ALL.h"

bool ArchiveMemory::CheckPage(unsigned int processID, unsigned int virtPage) { //Метод, проверяющий наличие страницы в ОП
	for (unsigned int i = 0; i < m_size; ++i) {
		if ((m_memory[i].m_processID == processID) && (m_memory[i].m_number == virtPage)) {
			return true;
		}
	}
	return false;
}

unsigned int ArchiveMemory::GetFreeSpace() { //Метод, проверяющий наличие страницы в ОП
	unsigned int j = 0;
	for (unsigned int i = 0; i < m_size; ++i) {
		if (m_memory[i].m_processID == -1) {
			j++;
		}
	}
	return j;
}

void ArchiveMemory::SavePage(unsigned int processID, unsigned int page) {
	for (unsigned int i = 0; i < m_size; ++i) {
		if (CheckPage(processID, page)) {
			return;
		}
		else {
			for (unsigned int i = 0; i < m_size; ++i) {
				if (m_memory[i].m_processID == -1) {
					m_memory[i].m_processID = processID;
					m_memory[i].m_number = page;
					return;
				}
			}
		}
	}
	assert(0);
}

void ArchiveMemory::LoadPageFromArch(unsigned int processID, unsigned int page) {
	for (unsigned int i = 0; i < m_size; ++i) {
		if ((m_memory[i].m_processID == processID) && (m_memory[i].m_number == page)) {
			m_memory[i].m_returns += 1;
			return;
		}
	}
	assert(0);
}

unsigned int ArchiveMemory::GetReturns(unsigned int processID, unsigned int page) {
	for (unsigned int i = 0; i < m_size; ++i) {
		if ((m_memory[i].m_processID == processID) && (m_memory[i].m_number == page)) {
			return m_memory[i].m_returns;
		}
	}
	return 0;
}

void ArchiveMemory::DeleteProcess(unsigned int processID) {
	for (unsigned int i = 0; i < m_size; ++i) {
		if (m_memory[i].m_processID == processID) {
			m_memory[i].m_processID = -1;
			m_memory[i].m_number = -1;
			m_memory[i].m_returns = 0;
		}
	}
}
