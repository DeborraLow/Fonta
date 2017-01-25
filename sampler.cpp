#include "sampler.h"

#include <cstdlib>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QXmlStreamReader>

#ifdef FONTA_MEASURES
#include <QElapsedTimer>
#include <QDebug>
#endif

#include "fontadb.h"

namespace fonta {

Sampler *Sampler::mInstance = nullptr;

Sampler *Sampler::instance() {
    if (mInstance == nullptr) {
        mInstance = new Sampler;
    }

    return mInstance;
}

Sampler::Sampler()
{
    // try to fetch rss news
    fetchNews(texts, "http://feeds.bbci.co.uk/news/world/rss.xml", "description");
    fetchNews(textsRus, "http://tass.ru/rss/v2.xml", "title");

    // filter font pair depending on users installed fonts
    const QStringList& families = fontaDB().families();

    for(const Sample& p : preSamples) {
        if(families.contains(p.family1) && families.contains(p.family2)) {
            samples << p;
        }
    }
}

const QStringList Sampler::names = {
    "Severin",
    "Alois",
    "Teo",
    "Tess",
    "Noel",
    "Noah",
    "Liam",
    "Alice",
    "Bob",
    "Aske",
    "Olga",
    "Tilda",
    "Vespa",
    "Solly",
    "Pit",
    "Kurt",
    "Sharona",
    "Melissa",
};

QStringList Sampler::texts = {
    "Before 1960 95% of soft drinks sold in the U.S. are furnished in reusable bottles."
    "Ernest Hemmingway commits suicide with shotgun.",
    "American U-2 spy plane, piloted by Francis Gary Powers, shot down over Russia",
    "Kennedy was assassinated in Dallas, Texas, on November 22, 1963",
    "Donald Trump promises to dissolve his Trump Foundation charity, which is still under investigation.",
};

QStringList Sampler::textsRus = {
    "Шифровальщица попросту забыла ряд ключевых множителей и тэгов",
    "Широкая электрификация южных губерний даст мощный толчок подъёму сельского хозяйства",
    "Подъём с затонувшего эсминца легкобьющейся древнегреческой амфоры сопряжён с техническими трудностями",
};

const QVector<Sample> Sampler::preSamples = {
    {
        "Georgia", 22,
        "Verdana", 11
    },
    {
        "Helvetica", 26,
        "Garamond", 12
    },
    {
        "Bodoni MT", 24,
        "FuturaLight", 16
    },
    {
        "Trebuchet MS", 18,
        "Verdana", 9
    },
    {
        "Century Schoolbook", 22,
        "Century Gothic", 12
    },
    {
        "Franklin Gothic Demi Cond", 24,
        "Century Gothic", 12
    },
    {
        "Tahoma", 18,
        "Segoe UI", 11
    },
    {
        "Franklin Gothic Demi", 20,
        "Trebuchet MS", 12
    },
    {
        "Trebuchet MS", 20,
        "Corbel", 11
    },
    {
        "Arial Black", 18,
        "Arial", 11
    },
    {
        "Impact", 22,
        "Arial Narrow", 12
    },
    {
        "Georgia", 20,
        "Calibri", 11
    },
    {
        "Segoe UI", 20,
        "Arial", 11
    },
    {
        "Terminal", 16,
        "Terminal", 16
    },
    {
        "Clarendon", 20,
        "Times New Roman", 12
    },
    {
        "Cooper Black", 22,
        "Trebuchet MS", 13
    },
};

struct NewsData {
    QStringList *list;
    QString tag;

    NewsData() {}
    NewsData(QStringList *list, const QString &tag)
        : list(list)
        , tag(tag)
#ifdef FONTA_MEASURES
        , timer(new QElapsedTimer)
#endif
    {}

#ifdef FONTA_MEASURES
    QElapsedTimer *timer;
#endif
};

QNetworkAccessManager *network;
static QHash<QNetworkReply *, NewsData> newsMap;

void Sampler::fetchNews(QStringList &list, CStringRef url, CStringRef tag)
{
    if(newsMap.isEmpty()) {
        network = new QNetworkAccessManager;
    }

    QNetworkReply *reply = network->get(QNetworkRequest(url));

    NewsData d(&list, tag);
    newsMap[reply] = d;

#ifdef FONTA_MEASURES
    d.timer->start();
#endif

    QObject::connect(reply, SIGNAL(finished()), this, SLOT(fetchNewsSlot()));
}

void cleanupNetwork(QNetworkReply *reply)
{
    reply->deleteLater();
    delete newsMap[reply].timer;
    newsMap.remove(reply);
    if(newsMap.isEmpty()) {
        delete network;
    }
}

void Sampler::fetchNewsSlot()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    NewsData &d = newsMap[reply];

    if(reply->error() != QNetworkReply::NoError) {
#ifdef FONTA_MEASURES
        qDebug() << d.timer->elapsed() << "ms: timeout to load news";
#endif
        cleanupNetwork(reply);
        return;
    } else {
#ifdef FONTA_MEASURES
        qDebug() << d.timer->elapsed() << "ms to load news";
#endif
    }

    QStringList &list = *d.list;
    CStringRef tag = d.tag;

    QStringList tmpList;

#ifdef FONTA_MEASURES
    d.timer->start();
#endif

    QXmlStreamReader r(reply->readAll());
    while(r.readNextStartElement());
    while(!r.atEnd()) {
        r.readNext();
        if(r.name() == "item") {
            while(!r.atEnd()) {
                r.readNext();
                if(r.name() == tag) {
                    tmpList << r.readElementText();
                    break;
                }
            }
        }
    }

    if(tmpList.count()) {
        list.clear();
        list << tmpList;
    }

#ifdef FONTA_MEASURES
    qDebug() << d.timer->elapsed() << "ms to process news rss-xml";
#endif

    cleanupNetwork(reply);
}

QVector<Sample> Sampler::samples;
QSet<int> Sampler::namesPool;
QSet<int> Sampler::textsPool;
QSet<int> Sampler::textsRusPool;
QSet<int> Sampler::samplesPool;

static int getPoolsValue(QSet<int>& pool, int length)
{    
    int r = rand()%length;

    if(!pool.contains(r)) {
        pool.insert(r);
        return r;
    } else {
        int i = r+1;
        while(i != r){
            if(i >= length) {
                i = 0;
                continue;
            }

            if(pool.contains(i)) {
                ++i;
            } else {
                pool.insert(i);
                return i;
            }
        }

        pool.clear();
        pool.insert(r);
        return r;
    }
}

CStringRef Sampler::getName()
{
    int i = getPoolsValue(namesPool, names.length());
    return names.at(i);
}

CStringRef Sampler::getText()
{
    int i = getPoolsValue(textsPool, texts.length());
    return texts.at(i);
}

CStringRef Sampler::getRusText()
{
    int i = getPoolsValue(textsRusPool, textsRus.length());
    return textsRus.at(i);
}

CStringRef Sampler::getTextForFamily(CStringRef family)
{
    if(fontaDB().isCyrillic(family)) {
        return getRusText();
    } else {
        return getText();
    }
}

void Sampler::loadSample(WorkArea& area)
{
    int i = getPoolsValue(samplesPool, samples.length());
    Sample& sample = samples[i];

    area.addField(InitType::Empty);
    area.addField(InitType::Empty);

    auto &field1 = *area.m_fields[0];
    auto &field2 = *area.m_fields[1];

    area.setSizes({120, 100});

    field1.setFontSize(sample.size1);
    field1.setSamples(getText(), getRusText());
    field1.setFontFamily(sample.family1);

    field2.setFontSize(sample.size2);
    field2.setSamples(getText(), getRusText());
    field2.setFontFamily(sample.family2);
}

} // namespace fonta
