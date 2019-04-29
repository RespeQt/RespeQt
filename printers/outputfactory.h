#ifndef OUTPUTFACTORY_H
#define OUTPUTFACTORY_H

#include <QVector>
#include <QString>
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
        typedef QPair<QString, Creator> CreatorPair;
        typedef QVector<CreatorPair> CreatorVector;
        CreatorVector creatorFunctions;

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
            creatorFunctions.append(CreatorPair(label, &creator<TDerived>));
        }

        NativeOutput* createOutput(QString label) const
        {
            typename CreatorVector::const_iterator it;
            for(it = creatorFunctions.begin(); it != creatorFunctions.end(); ++it)
            {
                if (it->first == label)
                {
                    return it->second();
                }
            }
            return Q_NULLPTR;
        }

        int numRegisteredOutputs() const
        {
            return creatorFunctions.size();
        }

        const QVector<QString> getOutputNames() const
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
#endif // OUTPUTFACTORY_H
