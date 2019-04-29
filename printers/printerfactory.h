#ifndef PRINTERFACTORY_H
#define PRINTERFACTORY_H

#include <QVector>
#include <QPair>
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
        typedef QPair<QString, Creator> CreatorPair;
        typedef QVector<CreatorPair> CreatorVector;
        CreatorVector creatorFunctions;

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
        void registerPrinter(QString label)
        {
            static_assert (std::is_base_of<BasePrinter, TDerived>::value, "PrinterFactory::registerPrinter doesn't accept this type because doesn't derive from base class");
            creatorFunctions.append(CreatorPair(label, &creator<TDerived>));
        }

        BasePrinter* createPrinter(QString label, SioWorker *worker) const
        {
            CreatorVector::const_iterator it;
            for(it = creatorFunctions.begin(); it != creatorFunctions.end(); ++it)
            {
                if (it->first == label)
                {
                    return it->second(worker);
                }
            }
            return Q_NULLPTR;
        }

        int numRegisteredPrinters() const
        {
            return creatorFunctions.size();
        }

        const QVector<QString> getPrinterNames() const
        {
            QVector<QString> names;
            CreatorVector::const_iterator it;
            for(it = creatorFunctions.begin(); it != creatorFunctions.end(); ++it)
            {
                names.append(it->first);
            }
            return names;
        }
    };
}
#endif // PRINTERFACTORY_H
