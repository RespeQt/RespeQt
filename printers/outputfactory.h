#ifndef OUTPUTFACTORY_H
#define OUTPUTFACTORY_H

#include <QMap>
#include <QString>
#include <QList>
#include "nativeoutput.h"

namespace Printers {

    class OutputFactory
    {
    private:
        template<class TDerived>
        static NativeOutput* creator()
        {
            return new TDerived();
        }

        // Instanciation maps
        typedef NativeOutput* (*Creator)();
        typedef QMap<QString, Creator> CreatorMap;
        CreatorMap creatorFunctions;

        static OutputFactory* sInstance;
        OutputFactory() {}

    public:
        static OutputFactory* instance()
        {
            if (sInstance == Q_NULLPTR)
            {
                sInstance = new OutputFactory();
            }
            return sInstance;
        }

        template<class TDerived>
        void registerOutput(QString label)
        {
            static_assert (std::is_base_of<NativeOutput, TDerived>::value, "OutputFactory::registerOutput doesn't accept this type because doesn't derive from base class");
            creatorFunctions[label] = &creator<TDerived>;
        }

        NativeOutput* createOutput(QString label) const
        {
            typename CreatorMap::const_iterator it = creatorFunctions.find(label);
            if (it != creatorFunctions.end())
            {
                return it.value()();
            }
            return Q_NULLPTR;
        }

        int numRegisteredOutputs() const
        {
            return creatorFunctions.size();
        }

        const QList<QString> getOutputNames() const
        {
            return creatorFunctions.keys();
        }

    };
}
#endif // OUTPUTFACTORY_H
