#ifndef PRINTERFACTORY_H
#define PRINTERFACTORY_H

#include <QVector>
#include <QPair>
#include <QString>
#include <memory>
#include "sioworker.h"
#include "baseprinter.h"

namespace Printers {

    class PrinterFactory
    {
    private:
        template<class TDerived>
        static BasePrinterPtr creator(SioWorkerPtr worker)
        {
            return std::make_shared<TDerived>(worker);
        }

        // Instanciation maps
        using Creator = BasePrinterPtr(SioWorkerPtr worker);
        using CreatorPair = std::pair<QString, Creator*>;
        using CreatorVector = std::vector<CreatorPair>;
        CreatorVector creatorFunctions;

        static std::unique_ptr<PrinterFactory> sInstance;
        PrinterFactory() {}

    public:
        static std::unique_ptr<PrinterFactory>& instance()
        {
            if (sInstance == nullptr)
            {
                sInstance.reset(new PrinterFactory());
            }
            return sInstance;
        }

        template<class TDerived>
        void registerPrinter(QString label)
        {
            static_assert(std::is_base_of<BasePrinter, TDerived>::value, "PrinterFactory::registerPrinter doesn't accept this type because it doesn't derive from base class");
            creatorFunctions.push_back(CreatorPair(label, &creator<TDerived>));
        }

        BasePrinterPtr createPrinter(QString label, SioWorkerPtr worker) const
        {
            for(auto it : creatorFunctions)
            {
                if (it.first == label)
                {
                    return it.second(worker);
                }
            }
            return nullptr;
        }

        int numRegisteredPrinters() const
        {
            return creatorFunctions.size();
        }

        const QVector<QString> getPrinterNames() const
        {
            QVector<QString> names;
            for(auto it : creatorFunctions)
            {
                names.append(it.first);
            }
            return names;
        }
    };
}
#endif // PRINTERFACTORY_H
