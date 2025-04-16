#include "ALL.h"
#include <random>

// Используем статическую переменную, чтобы инициализировать генератор
// только один раз во время первого вызова функции ChoosePage()
static std::mt19937 m_randomGenerator(100);

unsigned int Process::ChoosePage() {
	//Генерация случайного числа от 0 до 1
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	float random = dist(m_randomGenerator);
	float sum = 0.0f;
	for (unsigned int i = 0; i < m_processSize; ++i) {
		if (random <= (sum += m_distribution[i])) {
			// выбрана страница под номером i
			m_processPageUsageCount[i] += 1;
			return i;
		}
	}
	return 0;
}

OperationType Process::ChooseOperation() {
	//Генерация случайного числа от 0 до 1
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	float random = dist(m_randomGenerator);
	float sum = 0.0f;
	//Если случайное число меньше чем вероятность операции в регистре
	if (random <= (sum += m_opreg)) {
		// выбран тип операции m_opreg
		return OT_REG;
	}
	//Если случайное число больше чем вероятность операции в регистре и меньше чем сумма вероятности операции чтения и предыдущих вероятностей
	else if (random <= (sum += m_opread)) {
		// выбран тип операции m_opread
		return OT_READ;
	}
	//Если случайное число больше чем вероятность операции чтения и меньше чем сумма вероятности операции записи и предыдущих вероятностей
	else if (random <= (sum += m_opwrite)) {
		// выбран тип операции m_opwrite
		return OT_WRITE;
	}
	//Если случайное число больше чем вероятность операции записи и меньше чем сумма вероятности операции ввода/вывода и предыдущих вероятностей
	else {
		// выбран тип операции m_opinout
		return OT_IO;
	}
}
