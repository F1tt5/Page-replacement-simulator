#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "ALL.h"

using namespace std;

// ссылка на единственный объект симулятора
Sim* g_pSim;
OperatingSystem* OperatingSystem::g_pOperatingSystem = new OperatingSystem;

// Структура для хранения входных параметров
struct SimulationParams {
    // Основные параметры памяти
    unsigned int mainMemorySize;
    unsigned int archiveMemorySize;

    // Параметры процессора
    SimulatorTime timeForReg;
    SimulatorTime timeForRead;
    SimulatorTime timeForWrite;
    SimulatorTime timeForIO;
    unsigned int operationScaleNumber;

    // Параметры стратегии
    unsigned int strategy;
    SimulatorTime strategyTime;

    // Параметры времени
    SimulatorTime dispatchTime;
    SimulatorTime ioTime;
    SimulatorTime pageActionTime;

    // Общие параметры симуляции
    SimulatorTime TimeForWork;
    SimulatorTime quantTime;
    unsigned int infoSize;
    unsigned int output;
    unsigned int processesNumber;

    // Параметры процессов
    struct ProcessParams {
        unsigned int process_ID;
        unsigned int process_size;
        float register_operation_prob;
        float read_operation_prob;
        float write_operation_prob;
        float io_operation_prob;
        unsigned int distribution_type;
        SimulatorTime startTime;
        float distribution_parameter;
        vector<float> custom_distribution;
    };

    vector<ProcessParams> processes;

    // Параметры для удаления процессов
    vector<pair<unsigned int, SimulatorTime>> processRemovals;

    // Параметры итерации
    string iterationParameter;
    unsigned int iterationStart;
    unsigned int iterationEnd;
    unsigned int iterationStep;
};

// Функция для чтения параметров
SimulationParams ReadParameters() {
    SimulationParams params;
    std::string line;
    std::string nextline;
    unsigned int input = 0;
    unsigned int j = 0;

    while (std::getline(std::cin, line)) {
        // Игнорируем комментарии
        if (line[0] == '#' || line.empty()) continue;
        if (line[0] == '/') continue;

        if (line[0] == 'D') {
            input = 6;
            continue;
        }

        if (line[0] == 'I') {
            // Обработка параметров итерации
            if (line.find("ITERATIONS") != string::npos) {
                std::istringstream iss(line);
                string dummy;
                iss >> dummy >> params.iterationParameter >> params.iterationStart >> params.iterationEnd >> params.iterationStep;
            }
            continue;
        }

        std::istringstream iss(line);
        if (input == 6) {
            unsigned int process_ID;
            SimulatorTime Time;
            if (iss >> process_ID >> Time) {
                params.processRemovals.emplace_back(process_ID, Time);
            }
        }
        else if (input == 5) {
            SimulationParams::ProcessParams pp;
            float parameter = 1.0;

            if (iss >> pp.process_ID >> pp.process_size >> pp.register_operation_prob >>
                pp.read_operation_prob >> pp.write_operation_prob >> pp.io_operation_prob >>
                pp.distribution_type >> pp.startTime) {

                if (iss >> parameter) {
                    pp.distribution_parameter = parameter;
                }
                else {
                    // Устанавливаем разумные значения по умолчанию для разных типов распределений
                    switch (pp.distribution_type) {
                    case 1: // Uniform
                        pp.distribution_parameter = 0.0f;
                        break;
                    case 2: // Exponential
                        pp.distribution_parameter = 1.0f; // lambda = 1.0
                        break;
                    case 3: // Pareto
                        pp.distribution_parameter = 1.5f; // alpha = 1.5
                        break;
                    default:
                        pp.distribution_parameter = 1.0f;
                    }
                }

                if (pp.distribution_type == 4) {
                    std::getline(std::cin, nextline);
                    std::istringstream iss2(nextline);
                    float page_prob;
                    while (iss2 >> page_prob) {
                        pp.custom_distribution.push_back(page_prob);
                    }
                }

                params.processes.push_back(pp);
                j++;
            }
        }
        else if (input == 4) {
            if (iss >> params.TimeForWork >> params.quantTime >> params.infoSize >> params.output) {
                input++;
            }
        }
        else if (input == 3) {
            if (iss >> params.dispatchTime >> params.ioTime >> params.pageActionTime) {
                input++;
            }
        }
        else if (input == 2) {
            if (iss >> params.strategy >> params.strategyTime >> params.processesNumber) {
                input++;
            }
        }
        else if (input == 1) {
            if (iss >> params.timeForReg >> params.timeForRead >> params.timeForWrite >>
                params.timeForIO >> params.operationScaleNumber) {
                input++;
            }
        }
        else if (input == 0) {
            if (iss >> params.mainMemorySize >> params.archiveMemorySize) {
                input++;
            }
        }
    }

    return params;
}

// Функция для настройки симулятора с заданными параметрами
void SetupSimulation(const SimulationParams& params, int iteration = -1) {
    // Создаем файл для результатов
    std::string filename = "results.txt";
    filename = "results_" + to_string(iteration) + ".txt";

    std::ofstream file(filename);
    //if (!file.is_open()) return;

    // Создаем объекты симулятора
    MainMemory* mainMemory = new MainMemory(params.mainMemorySize);
    ArchiveMemory* archiveMemory = new ArchiveMemory(params.archiveMemorySize);
    Processor* processor = new Processor(params.timeForReg, params.timeForRead, params.timeForWrite);
    IODevice* iodevice = new IODevice(params.timeForIO);
    VirtualMemoryProcessor* VMP = new VirtualMemoryProcessor();

    // Настраиваем операционную систему
    OperatingSystem::g_pOperatingSystem->SetMainMemory(mainMemory);
    OperatingSystem::g_pOperatingSystem->SetArchiveMemory(archiveMemory);
    OperatingSystem::g_pOperatingSystem->SetProcessor(processor);
    OperatingSystem::g_pOperatingSystem->SetIODevice(iodevice);
    OperatingSystem::g_pOperatingSystem->SetVMP(VMP);
    OperatingSystem::g_pOperatingSystem->m_Scale = params.operationScaleNumber;

    // Настраиваем стратегию
    OperatingSystem::g_pOperatingSystem->ChooseStrategy(params.strategy);
    OperatingSystem::g_pOperatingSystem->GetStrategy()->SetResetTime(params.strategyTime);

    string strategyName = OperatingSystem::g_pOperatingSystem->GetStrategy()->GetName();
    file << "Selected strategy: " << strategyName << "\n";
    if (strategyName == "NRU") {
        file << "Reset time = " << OperatingSystem::g_pOperatingSystem->GetStrategy()->GetResetTime() << "\n";
    }
    else if (strategyName == "Aging") {
        file << "Aging time = " << OperatingSystem::g_pOperatingSystem->GetStrategy()->GetResetTime() << "\n";
    }

    // Настраиваем временные параметры
    OperatingSystem::g_pOperatingSystem->m_DispatchTime = params.dispatchTime;
    OperatingSystem::g_pOperatingSystem->m_IOTime = params.ioTime;
    OperatingSystem::g_pOperatingSystem->m_PageActionTime = params.pageActionTime;

    // Настраиваем процессы
    OperatingSystem::g_pOperatingSystem->m_processesSize = params.processes.size();
    OperatingSystem::g_pOperatingSystem->m_processes = new Process * [params.processes.size()];

    for (size_t i = 0; i < params.processes.size(); ++i) {
        const auto& pp = params.processes[i];
        file << "Process " << pp.process_ID << ":\n";
        file << "Size: " << pp.process_size << " pages\n";
        file << "Operations prob: Register=" << pp.register_operation_prob << " Read=" << pp.read_operation_prob << " Write=" << pp.write_operation_prob << " IO=" << pp.io_operation_prob << "\n";

        float* distribution = nullptr;
        if (pp.distribution_type == 4) {
            distribution = new float[pp.process_size];
            for (size_t j = 0; j < pp.custom_distribution.size() && j < pp.process_size; ++j) {
                distribution[j] = pp.custom_distribution[j];
            }
        }
        else {
            distribution = OperatingSystem::g_pOperatingSystem->ChooseDistribution(pp.process_size, pp.distribution_type, pp.distribution_parameter);
        }

        file << "Distribution: ";
        for (unsigned int j = 0; j < pp.process_size; ++j) {
            file << distribution[j] << " ";
        }
        file << "\n\n";

        OperatingSystem::g_pOperatingSystem->m_processes[i] = new Process(
            pp.process_ID, pp.process_size, pp.register_operation_prob,
            pp.read_operation_prob, pp.write_operation_prob, pp.io_operation_prob,
            distribution);

        OperatingSystem::g_pOperatingSystem->m_processes[i]->SetQuant(params.quantTime);
        delete[] distribution;

        if (pp.startTime == 0) {
            OperatingSystem::g_pOperatingSystem->m_Active->Add(pp.process_ID);
        }
        else {
            Schedule(pp.startTime, OperatingSystem::g_pOperatingSystem, &OperatingSystem::StartProcess, static_cast<unsigned int>(i));
            OperatingSystem::g_pOperatingSystem->m_processes[i]->m_TimeStartWaiting = pp.startTime;
        }
    }

    // Настраиваем удаление процессов
    for (const auto& removal : params.processRemovals) {
        Schedule(removal.second, OperatingSystem::g_pOperatingSystem, &OperatingSystem::RemoveProcess, removal.first);
    }

    if (strategyName == "NRU") {
        Schedule(g_pSim->GetTime() + OperatingSystem::g_pOperatingSystem->GetStrategy()->GetResetTime(), OperatingSystem::g_pOperatingSystem, &OperatingSystem::ResetBitR);
    } else if (strategyName == "Aging") {
        Schedule(g_pSim->GetTime() + OperatingSystem::g_pOperatingSystem->GetStrategy()->GetResetTime(), OperatingSystem::g_pOperatingSystem, &OperatingSystem::UpdateAgeCounters);
    }
    

    // Настраиваем сбор информации
    OperatingSystem::g_pOperatingSystem->SetInfoSize(params.infoSize);
    SimulatorTime time = (1000000000 * params.TimeForWork) / params.infoSize;
    Schedule(g_pSim->GetTime()+1, OperatingSystem::g_pOperatingSystem, &OperatingSystem::CollectInfo, 0, time, params.infoSize);

    OperatingSystem::g_pOperatingSystem->m_output = params.output;
    OperatingSystem::g_pOperatingSystem->m_quant = params.quantTime;


    file.close();
}


int main() {
    // Считываем параметры
    SimulationParams params = ReadParameters();

    // Создаем вектор значений для итерации
    vector<unsigned int> iterationValues;
    if (!params.iterationParameter.empty()) {
        for (unsigned int val = params.iterationStart; val <= params.iterationEnd; val += params.iterationStep) {
            iterationValues.push_back(val);
        }
    }
    else {
        // Если параметры итерации не заданы, используем значение по умолчанию (один прогон)
        iterationValues.push_back(0);
    }

    // Цикл по разным значениям параметра
    for (size_t i = 0; i < iterationValues.size(); ++i) {
        // Создаем новый объект симулятора для каждой итерации
        g_pSim = new Sim;
        OperatingSystem::g_pOperatingSystem = new OperatingSystem;

        // Модифицируем параметр только если итерации заданы
        if (!params.iterationParameter.empty()) {
            if (params.iterationParameter == "mainMemorySize") {
                params.mainMemorySize = iterationValues[i];
            }
        }

        std::string journalName = "Journal_" + std::to_string(i) + ".txt";
        std::ofstream journal(journalName);

        if (!journal.is_open()) {
            std::cerr << "Failed to create " << journalName << std::endl;
            continue;
        }

        // Перенаправляем вывод
        std::streambuf* old_cout = std::cout.rdbuf();
        std::cout.rdbuf(journal.rdbuf());

        // Настраиваем симулятор
        SetupSimulation(params, i);

        // Устанавливаем имя агента
        OperatingSystem::g_pOperatingSystem->SetName("OS");

        // Запускаем агент
        OperatingSystem::g_pOperatingSystem->Start();

        // Устанавливаем предел времени симуляции
        g_pSim->SetLimit(1000000000 * params.TimeForWork + 1);

        // Запускаем симулятор
        while (!g_pSim->Run()) {
            if (g_pSim->GetTime() >= (1000000000 * params.TimeForWork + 1)) {
                break;
            }
        }

        // Сохраняем результаты



        SimulatorTime WorkTime = 0;
        long long OperationsDone = 0;

        std::string filename = "results_" + to_string(i) + ".txt";
        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {

            // Записываем параметры в файл
            file << "Simulation parameters:\n";
            file << "Main memory size: " << params.mainMemorySize << " pages\n";
            file << "Archive memory size: " << params.archiveMemorySize << " pages\n";
            file << "Time for register: " << params.timeForReg << " ns\n";
            file << "Time for read: " << params.timeForRead << " ns\n";
            file << "Time for write: " << params.timeForWrite << " ns\n";
            file << "Time for IO: " << params.timeForIO << " ns\n";
            file << "Operation scale: " << params.operationScaleNumber << "\n";
            file << "Strategy: " << OperatingSystem::g_pOperatingSystem->GetStrategy()->GetName() << "\n";
            file << "Processes number: " << params.processesNumber << "\n";
            file << "Dispatch time: " << params.dispatchTime << " ns\n";
            file << "IO time: " << params.ioTime << " ns\n";
            file << "Page action time: " << params.pageActionTime << " ns\n";
            file << "Work time: " << params.TimeForWork << " s\n";
            file << "Quant time: " << params.quantTime << " ns\n";
            file << "Info size: " << params.infoSize << "\n";
            file << "Output: " << params.output << "\n\n";

            for (unsigned int i = 0; i < OperatingSystem::g_pOperatingSystem->m_processesSize; ++i) {
                file << "Information about process " << i + 1 << "\n";
                Process* process = OperatingSystem::g_pOperatingSystem->m_processes[i];
                long long opReg = process->m_operationsDone[0];
                long long opRead = process->m_operationsDone[1];
                long long opWrite = process->m_operationsDone[2];
                long long opIO = process->m_operationsDone[3];
                long long sum = opReg + opRead + opWrite + opIO;
                file << "Operations done for Process: Reg., Read, Write, IO, Sum\n";
                file << opReg << " " << opRead << " " << opWrite << " " << opIO << " " << sum << "\n";
                OperationsDone += sum;
                file << "WorkTime for process = " << process->m_WorkTime << "\n";
                WorkTime += process->m_WorkTime;
                file << "AllTime for process = " << process->m_AllTime << "\n";
                file << "Overhead expenses = " << (1.0f - (static_cast<float>(process->m_WorkTime) / process->m_AllTime)) * 100.0f << "%" << "\n";
                file << "Page faults for process = " << process->m_PageFaults << "\n";
                file << "Save pages for process = " << process->m_PageSaves << "\n" << "\n";

                for (unsigned int j = 0; j < process->GetSize(); ++j) {
                    process->m_processPageComeBacksCount[j] += OperatingSystem::g_pOperatingSystem->m_archiveMemory->GetReturns(process->GetProcessID(), j);
                }
                file << "Information about pages: number, Choose probability, Number of usage, Number of loads:" << "\n";
                for (unsigned int j = 0; j < process->GetSize(); ++j) {
                    file << j << " " << process->m_distribution[j] << " " << process->m_processPageUsageCount[j] << " " << process->m_processPageComeBacksCount[j] << "\n";
                }
                file << "\n";
            }

            file << "\n" << "Information about system:" << "\n";
            file << "Page faults for system = " << OperatingSystem::g_pOperatingSystem->m_PageFaults << "\n";
            file << "Save pages for system = " << OperatingSystem::g_pOperatingSystem->m_PageSaves << "\n";
            file << "Positive work time for system = " << WorkTime << "\n";
            file << "All work time for system = " << g_pSim->GetTime() - 1 << "\n";
            file << "Operations in 1 second done = " << OperationsDone / static_cast<long long>(params.TimeForWork) << "\n";
            file << "Overhead expenses = " << (1.0f - (static_cast<float>(WorkTime) / (1000000000 * params.TimeForWork))) * 100.0 << "%" << "\n";

            file << "Time, Overhead expenses, OP free space, AM free space:" << "\n";
            for (unsigned int i = 0; i < params.infoSize; ++i) {
                file << OperatingSystem::g_pOperatingSystem->m_InfoTable[i].m_time << " " << OperatingSystem::g_pOperatingSystem->m_InfoTable[i].m_OverheadExpenses << "% " << OperatingSystem::g_pOperatingSystem->m_InfoTable[i].m_OPspace << " " << OperatingSystem::g_pOperatingSystem->m_InfoTable[i].m_AMspace << "\n";
            }
            file.close();
        }

        // Удаляем объекты симулятора
        delete OperatingSystem::g_pOperatingSystem;
        delete g_pSim;
    }

    return 0;
}