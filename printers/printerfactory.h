#ifndef PRINTERFACTORY_H
#define PRINTERFACTORY_H

#include <QMap>
#include <QString>
#include "sioworker.h"
#include "baseprinter.h"

namespace Printers {

    class PrinterFactory
    {
    private:
        template<class TDerived>
        static BasePrinter* creator(SioWorker *worker)
        {
            return new TDerived(worker);
        }

        // Instanciation maps
        typedef BasePrinter* (*Creator)(SioWorker *worker);
        typedef QMap<int, Creator> CreatorMap;
        CreatorMap creatorFunctions;

        // Label maps
        typedef QMap<int, QString> LabelMap;
        LabelMap printerLabels;

        static PrinterFactory* sInstance;
        PrinterFactory() {}

    public:
        static PrinterFactory* instance()
        {
            if (sInstance == Q_NULLPTR)
            {
                sInstance = new PrinterFactory();
            }
            return sInstance;
        }

        template<class TDerived>
        void registerPrinter(int type, QString label)
        {
            static_assert (std::is_base_of<BasePrinter, TDerived>::value, "PrinterFactory::registerPrinter doesn't accept this type because doesn't derive from base class");
            creatorFunctions[type] = &creator<TDerived>;
            printerLabels[type] = label;
        }

        BasePrinter* createPrinter(int type, SioWorker *worker) const
        {
            typename CreatorMap::const_iterator it = creatorFunctions.find(type);
            if (it != creatorFunctions.end())
            {
                return it.value()(worker);
            }
            return Q_NULLPTR;
        }

        QString printerLabel(int type) const
        {
            typename LabelMap::const_iterator it = printerLabels.find(type);
            if (it != printerLabels.end())
            {
                return it.value();
            }
            return Q_NULLPTR;
        }

        int numRegisteredPrinters() const
        {
            return creatorFunctions.size();
        }
    };
}
#endif // PRINTERFACTORY_H
