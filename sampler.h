#ifndef SAMPLER_H
#define SAMPLER_H

#include <QSet>
#include <QStringList>
#include <fontawidgets.h>

namespace fonta {

typedef void (*SampleLoader)(WorkArea&);

class Sampler;

struct Sample {
    QString family1;
    int size1;
    QString family2;
    int size2;
};

class Sampler : public QObject
{
    Q_OBJECT
public:
    static Sampler *instance();

    static CStringRef getName();
    static CStringRef getEngText(ContentMode mode);
    static CStringRef getRusText(ContentMode mode);
    static CStringRef getTextForFamily(CStringRef family, ContentMode mode);
    static void loadSample(WorkArea& area);

private slots:
    void fetchNewsSlot();

private:
    Sampler();
    Sampler(const Sampler &) = delete;
    void operator=(const Sampler &) = delete;
    static Sampler *mInstance;

    void fetchNews(QStringList &list, CStringRef url, CStringRef tag);

    static const QStringList names;
    static QStringList texts;
    static QStringList textsRus;
    static const QVector<Sample> preSamples;
    static QVector<Sample> samples;


    static QSet<int> namesPool;
    static QSet<int> textsPool;
    static QSet<int> textsRusPool;
    static QSet<int> samplesPool;
};

} // namespace fonta

#endif // SAMPLER_H
