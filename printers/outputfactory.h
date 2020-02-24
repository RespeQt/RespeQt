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
        static NativeOutputPtr creator()
        {
            return std::make_shared<TDerived>();
        }

        // Instanciation maps
        using Creator = NativeOutputPtr(); // Function type definition
        using CreatorPair = QPair<QString, Creator*>;
        using CreatorVector = QVector<CreatorPair>;
        CreatorVector creatorFunctions;

        static std::unique_ptr<OutputFactory> sInstance;
        OutputFactory() {}

    public:
        static std::unique_ptr<OutputFactory>& instance()
        {
            if (sInstance == nullptr)
            {
                sInstance.reset(new OutputFactory());
            }
            return sInstance;
        }

        template<class TDerived>
        void registerOutput(QString label)
        {
            static_assert (std::is_base_of<NativeOutput, TDerived>::value, "OutputFactory::registerOutput doesn't accept this type because doesn't derive from base class");
            creatorFunctions.append(CreatorPair(label, &creator<TDerived>));
        }

        NativeOutputPtr createOutput(QString label) const
        {
            for(auto it : creatorFunctions)
            {
                if (it.first == label)
                {
                    return it.second();
                }
            }
            return nullptr;
        }

        int numRegisteredOutputs() const
        {
            return creatorFunctions.size();
        }

        const QVector<QString> getOutputNames() const
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
#endif // OUTPUTFACTORY_H
