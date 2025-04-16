#pragma once
#include <cmath>

class PageDistribution
{
public:
    PageDistribution() {}
    virtual ~PageDistribution() {}

    virtual const char* GetName() = 0;
    virtual float* CreateDistribution(unsigned int pagesNumber, float parameter) = 0;
};



class Uniform : public PageDistribution {
public:
    const char* GetName() override
    {
        return "Uniform";
    }
    float* CreateDistribution(unsigned int pagesNumber, float parameter) override
    {
        // Определяем вероятность
        float probability = 1.0f / pagesNumber;
        // Создаем массив вероятностей
        float* distribution = new float[pagesNumber];
        for (unsigned int i = 0; i < pagesNumber; ++i) {
            distribution[i] = probability;
        }
        //Возвращаем данный массив
        return distribution;
    }
};

/*class Exponential : public PageDistribution {
public:
    const char* GetName() override
    {
        return "Exponential";
    }
    float* CreateDistribution(unsigned int pagesNumber, float parameter) override
    {
        // Создаем массив вероятностей
        float* distribution = new float[pagesNumber];
        // Задаём вероятности
        for (unsigned int i = 0; i < pagesNumber; ++i) {
            distribution[i] = exp(-parameter * i);
        }
        // Нормализуем массив вероятностей
        float sum = 0;
        for (unsigned int i = 0; i < pagesNumber; i++) {
            sum += distribution[i];
        }

        for (unsigned int i = 0; i < pagesNumber; i++) {
            distribution[i] /= sum;
        }
        //Возвращаем данный массив
        return distribution;
    }
};

class Pareto : public PageDistribution {
public:
    const char* GetName() override
    {
        return "Pareto";
    }

    float* CreateDistribution(unsigned int pagesNumber, float parameter) override
    {
        // Параметр распределения Парето
        const float pareto_alpha = 1.5f;
        // Нормирующая константа
        const float pareto_c = 1.0f / (powf(pareto_alpha, pareto_alpha) * expf(lgammaf(1.0f + 1.0f / pareto_alpha)));
        // Создаём массив с рампределением
        float* distribution = new float[pagesNumber];

        // Задаем вероятности
        for (unsigned int i = 0; i < pagesNumber; ++i) {
            distribution[i] = pareto_c / pow(i + 1.0f, pareto_alpha);
        }

        // Нормализуем массив вероятностей
        float sum = 0;
        for (unsigned int i = 0; i < pagesNumber; i++) {
            sum += distribution[i];
        }

        for (unsigned int i = 0; i < pagesNumber; i++) {
            distribution[i] /= sum;
        }
        //В данном методе у нас есть нормирующая константа, так что отдельное нормирование не требуется
        // Возвращаем данный массив
        return distribution;
    }
};*/

class Exponential : public PageDistribution {
public:
    const char* GetName() override { return "Exponential"; }

    float* CreateDistribution(unsigned int pagesNumber, float lambda) override {
        if (lambda <= 0) lambda = 1.0f; // Защита от неположительных значений

        float* distribution = new float[pagesNumber];
        float sum = 0.0f;

        for (unsigned int i = 0; i < pagesNumber; ++i) {
            distribution[i] = exp(-lambda * i);
            sum += distribution[i];
        }

        // Нормализация
        if (sum > 0) {
            for (unsigned int i = 0; i < pagesNumber; ++i) {
                distribution[i] /= sum;
            }
        }

        return distribution;
    }
};

class Pareto : public PageDistribution {
public:
    const char* GetName() override { return "Pareto"; }

    float* CreateDistribution(unsigned int pagesNumber, float alpha) override {
        if (alpha <= 0) alpha = 1.5f; // Значение по умолчанию для Парето

        float* distribution = new float[pagesNumber];
        float sum = 0.0f;

        for (unsigned int i = 0; i < pagesNumber; ++i) {
            distribution[i] = 1.0f / pow(i + 1.0f, alpha); // +1 чтобы избежать деления на 0
            sum += distribution[i];
        }

        // Нормализация
        if (sum > 0) {
            for (unsigned int i = 0; i < pagesNumber; ++i) {
                distribution[i] /= sum;
            }
        }

        return distribution;
    }
};
