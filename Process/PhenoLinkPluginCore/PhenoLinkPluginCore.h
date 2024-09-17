#ifndef CHECKOUT_PLUGIN_CORE_H_
#define CHECKOUT_PLUGIN_CORE_H_


#include <QtPlugin>
#include <QString>

#include "Core/Dll.h"
#include "Core/checkoutprocessplugininterface.h"

#include <queue>
#include <vector>
#include <algorithm>

#include <opencv2/opencv.hpp>

class DllPluginExport PhenoLinkPluginCore : public CheckoutProcessPluginInterface
{

public:
    inline cv::Mat stitch(ImageXP& img, int chann = 0, int* ext_nrows = NULL, int* ext_ncols = NULL)
    {

        QSet<QString> col_counter;
        for (unsigned i = 0; i < img.count(); ++i)
            col_counter.insert(getMetaInformation(&img).getMeta(QString("f%1X").arg(i),false));


        int ncols = col_counter.size();

        if (ncols == 1 && img.count() == 1)
            return img.getImage(0, chann);

        int Nrows = std::max(1., (ceil(img.count() / (double)ncols)));

        int rows = 0, cols = 0;
        std::vector<cv::Mat > im(img.count());
        for (unsigned i = 0; i < img.count(); ++i)
        {
            im[i] = img.getImage(i, chann);
            rows = std::max(rows, im[i].rows);
            cols = std::max(cols, im[i].cols);
        }


        if (ext_nrows) *ext_nrows = rows;
        if (ext_ncols) *ext_ncols = cols;

        cv::Mat res = cv::Mat::zeros(rows * Nrows, cols * ncols, CV_16U);

//        qDebug() << Nrows << ncols << res.rows << res.cols;

        for (int k = 0, i = 0; i < Nrows; i++) {
            int y = i * rows;
            int x_end = 0;
            for (int j = 0; j < ncols && k < img.count(); k++, j++) {
                int x = x_end;
                cv::Rect roi(x, y, cols, rows);
                cv::Size s = res(roi).size();

                cv::Mat simg = im[k];

                if (!simg.empty())
                    simg.copyTo(res(roi));

                x_end += cols;
            }
        }

        return res;
    }

    enum eProjection { Project_Max, Project_Min, Project_Avg /* Mean */};

    inline cv::Mat stitch(StackedImageXP& img, int chann = 0, bool ext_semaphore = false,
                          eProjection proj = Project_Max, int* ext_nrows = NULL, int* ext_ncols = NULL)
    {

        QSet<QString> col_counter;
        for (unsigned i = 0; i < img.count(); ++i)
            col_counter.insert(getMetaInformation(&img).getMeta(QString("f%1X").arg(i)));


        int ncols = col_counter.size();

        if (ncols == 1 && img.count() == 1)
            return img.getImage(0).getImage(0, chann);

        int Nrows = std::max(1., (ceil(img.count() / (double)ncols)));

        // Load images to get the images sizes
        int rows = 0, cols = 0;


        std::vector<cv::Mat > im(img.count());

        for (unsigned i = 0; i < img.count(); ++i)
        {
            im[i] = img.getImage(i).getImage(0, chann, QString(), !ext_semaphore);
            rows = std::max(rows, im[i].rows);
            cols = std::max(cols, im[i].cols);

        }


        if (ext_nrows) *ext_nrows = rows;
        if (ext_ncols) *ext_ncols = cols;

        cv::Mat res = cv::Mat::zeros(rows * Nrows, cols * ncols, CV_16U);

      //  qDebug() << Nrows << ncols << res.rows << res.cols;

        for (int k = 0, i = 0; i < Nrows; i++) {
            int y = i * rows;
            int x_end = 0;
            for (int j = 0; j < ncols && k < img.count(); k++, j++) {
                int x = x_end;
                cv::Rect roi(x, y, cols, rows);
                cv::Size s = res(roi).size();

                cv::Mat simg;
                if (img.count() == 1)
                    simg = im[k];
                else
                {
                    simg = im[k];
                    int avg_c = 0;
                    for (int st = 1; st < img.getImage(k).count(); ++st)
                    {
                        cv::Mat t = img.getImage(k).getImage(st, chann, QString(), !ext_semaphore);
                        if (!t.empty())
                        {
                            switch (proj)
                            {
                            case Project_Max:
                                cv::max(simg, t, simg);
                                break;
                            case Project_Min:
                                cv::min(simg, t, simg); break;
                            case Project_Avg:
                                simg += t;
                                avg_c++;
                                break;
                            }

                        }
                        if (proj == Project_Avg)
                            simg /= avg_c;
                    }
                    if (!simg.empty())
                        simg.copyTo(res(roi));

                    x_end += cols;
                }
            }
        }
        return res;
    }

    inline cv::Mat stitch(TimeImageXP& img, int chann = 0, int timepoint = 0,  int* ext_nrows = NULL, int* ext_ncols = NULL)
    {

        QSet<QString> col_counter;
        for (unsigned i = 0; i < img.count(); ++i)
            col_counter.insert(getMetaInformation(&img).getMeta(QString("f%1X").arg(i), false));


        int ncols = col_counter.size();

        if (ncols == 1 && img.count() == 1)
            return img.getImage(0, QString(), true).getImage(timepoint, chann);

        size_t Nrows = std::max(1., (ceil(img.count() / (double)ncols)));

        int rows = 0, cols = 0;

        std::vector<cv::Mat > im(img.count());

        for (size_t i = 0; i < img.count(); ++i)
        {
            im[i] = img.getImage(i, QString(), true).getImage(timepoint, chann);
            rows = std::max(rows, im[i].rows);
            cols = std::max(cols, im[i].cols);
        }


        if (ext_nrows) *ext_nrows = rows;
        if (ext_ncols) *ext_ncols = cols;

        cv::Mat res = cv::Mat::zeros(rows * Nrows, cols * ncols, CV_16U);

//        qDebug() << Nrows << ncols << res.rows << res.cols;

        for (size_t k = 0, i = 0; i < Nrows; i++) {
            int y = i * rows;
            int x_end = 0;
            for (int j = 0; j < ncols && k < img.count(); k++, j++) {
                int x = x_end;
                cv::Rect roi(x, y, cols, rows);
//                cv::Size s = res(roi).size();

                cv::Mat simg = im[k];

                if (!simg.empty())
                    simg.copyTo(res(roi));

                x_end += cols;
            }
        }
        return res;
    }

    QString buildTime() const
    {
        return QString("%1 - %2").arg(__DATE__).arg(__TIME__);
    }


};


// To compute standard statistics with partial/incomplete data (std + means of subsets)
struct UpdatableStats
{

    UpdatableStats(bool with_median = false) : _mean(0.0), variance(0.0), M2(0.0), count(0.0), med(with_median), _median(0.0),
        _min(std::numeric_limits<double>::max()),
        _max(std::numeric_limits<double>::min())
    {
    }




    inline void update(double newValue)
    {
        count += 1;
        double delta = newValue - _mean;

        _mean += delta / count;
        double delta2 = newValue - _mean;
        M2 += delta * delta2;

        if (newValue < _min)
            _min = newValue;
        else if (newValue > _max)
            _max = newValue;


        if (med)
        {
            updateMed(newValue);
        }
    }


    inline void updateMed(double newValue)
    {
        if (s.size() == 0)
        {
            _median = newValue;
            s.push(newValue);
        }
        else
        {
            if (s.size() > g.size())
            {
                if (newValue < _median)
                {
                    g.push(s.top());
                    s.pop();
                    s.push(newValue);
                }
                else
                    g.push(newValue);
                _median = (s.top() + g.top()) / 2.0;
            }
            else if (s.size() == g.size())
            {
                if (newValue < _median)
                {
                    s.push(newValue);
                    _median = s.top();
                }
                else
                {
                    g.push(newValue);
                    _median = g.top();
                }
            }
            else
            {
                if (newValue > _median)
                {
                    s.push(g.top());
                    g.pop();
                    g.push(newValue);
                }
                else
                    s.push(newValue);
                _median = (s.top() + g.top()) / 2.0;
            }
        }
    }

    inline double& min()
    {
        return _min;
    }

    inline double& max()
    {
        return _max;
    }

    inline double& mean()
    {
        return _mean;
    }

    inline double& std()
    {
        _std = sqrt(M2 / count);
        return _std;
    }

    inline double& sampleStd()
    {
        _sampStd = sqrt(M2 / (count - 1));
        return _sampStd;
    }

    inline double& median()
    {
        return _median;
    }


    inline UpdatableStats& operator+=(const UpdatableStats& lhs) // Fuse two Updatable Obj :)
    {

        double c = count + lhs.count;
        double delta = lhs._mean - _mean;
        double lM2 = M2 + lhs.M2 + ((delta * delta) * lhs.count * count) / c;

        this->variance = M2 / (c - 1);
        this->count = c;
        this->M2 = lM2;
        this->_mean = _mean + ((delta * lhs.count) / c);

        this->_min = std::min(this->_min, lhs._min);
        this->_max = std::max(this->_max, lhs._max);


        if (med && lhs.med)
        {
            auto ls = lhs.s;
            auto lg = lhs.g;

            while (ls.size() || lg.size())
            {
                if (ls.size())
                {
                    auto x = ls.top();
                    ls.pop();
                    this->updateMed(x);
                }
                if (lg.size())
                {
                    auto x = lg.top();
                    lg.pop();
                    this->updateMed(x);
                }
            }
        }

        return *this;
    }



    std::priority_queue<double> s;
    std::priority_queue<double, std::vector<double>, std::greater<double> > g;


    double _mean, variance, M2, count;
    double _std, _sampStd;

    bool med;
    double _median;

    double _min, _max;

};

struct SegmentNucleiOptions
{
    SegmentNucleiOptions(): kernel_size(5), min_radius(121),
        dilation_size(1), threshold(0), min_area(1000), max_area(std::numeric_limits<int>::max()),
        blur_size(0), relabel(false)
    {}

    int kernel_size;
    int min_radius;
    int dilation_size;
    int threshold;
    int min_area, max_area;
    int blur_size;

    bool relabel;
};

inline cv::Rect rect(cv::Mat& stats, int nuc)
{
    return cv::Rect(stats.at<int>(nuc, cv::CC_STAT_LEFT),
                    stats.at<int>(nuc, cv::CC_STAT_TOP),
                    stats.at<int>(nuc, cv::CC_STAT_WIDTH),
                    stats.at<int>(nuc, cv::CC_STAT_HEIGHT));
}

inline cv::Rect center(cv::Mat& stats, int nuc)
{
    return cv::Rect((int)floor(stats.at<int>(nuc, cv::CC_STAT_LEFT)+stats.at<int>(nuc, cv::CC_STAT_WIDTH)/2.),
                    (int)floor(stats.at<int>(nuc, cv::CC_STAT_TOP)+stats.at<int>(nuc, cv::CC_STAT_HEIGHT)/2.),
                    1,
                    1);
}


inline void NonMaximaSuppression(const cv::Mat& src, cv::Mat& mask, unsigned minRadius)
{
    cv::Mat element = getStructuringElement(cv::MORPH_RECT,
        cv::Size(minRadius, minRadius),
        cv::Point(1, 1));
    // find pixels that are equal to the local neighborhood not maximum (including ' ')
    cv::dilate(src, mask, element);
    cv::compare(src, mask, mask, cv::CMP_GE);

    cv::Mat non_plateau_mask;
    cv::erode(src, non_plateau_mask, element);
    cv::compare(src, non_plateau_mask, non_plateau_mask, cv::CMP_GT);
    cv::bitwise_and(mask, non_plateau_mask, mask);
}

template <class Type>
inline double clearOversizeArea(cv::Mat_<Type>& labels, cv::Mat& stats, double min, double max)
{
    double avg_area = 0, count_compo = 0;
    int compos = stats.rows;
    for (int j = 1; j < compos; ++j) // skip 0 as 0 is the background
    {
        if (stats.at<int>(j, cv::CC_STAT_AREA) < min ||
                stats.at<int>(j, cv::CC_STAT_AREA) > max )
        {
            if (stats.at<int>(j, cv::CC_STAT_AREA) > max)
                qDebug() << "Oversized Area:" << stats.at<int>(j, cv::CC_STAT_AREA);
            cv::Rect roi = rect(stats, j);
            cv::Mat sub=labels(roi);
            sub.setTo(0, sub == j);
        }
        else
        {
            count_compo++;
            avg_area += stats.at<int>(j, cv::CC_STAT_AREA);
        }
    }
    avg_area /= count_compo;
    return avg_area;
}

template <class Type>
inline cv::Mat SegmentNucleiWathershed(cv::Mat_<Type>& nuc_image, SegmentNucleiOptions& options)
{
    cv::Mat blur;
    if (options.blur_size > 0)
        cv::GaussianBlur(nuc_image, blur, cv::Size(0,0), options.blur_size, options.blur_size);
    else
        blur = nuc_image.clone(); // Explicit cloning ?

    cv::Mat labels, stats;
    cv::Mat1d centroids;

    cv::connectedComponentsWithStats(blur > options.threshold, labels, stats, centroids, 4, CV_32S, cv::CCL_WU);

    double avg_area = clearOversizeArea((cv::Mat_<int>&)labels, stats, options.min_area, options.max_area);

    cv::Mat distance;
    labels.convertTo(labels, CV_8U);
    cv::distanceTransform(labels, distance, 2, options.kernel_size);
    NonMaximaSuppression(distance, labels, options.min_radius);
    distance.release();

    if (options.dilation_size > 0)
    {
        cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * options.dilation_size + 1, 2 * options.dilation_size + 1), cv::Point(options.dilation_size, options.dilation_size));
        dilate(labels, labels, element);
        element.release();
    }

    // Relabel
    cv::connectedComponents(labels.clone(), labels, 4, CV_32S);
    labels.setTo(-1, blur < options.threshold);

    {
        cv::Mat fused;
        labels.convertTo(fused, CV_8U);
        std::vector<cv::Mat> vec = { fused, fused, fused };
        cv::merge(vec, fused);

        cv::watershed(fused, labels);
    }

    labels.setTo(0, labels < 0);
    cv::Mat con;
    labels.convertTo(con, CV_8U);
    cv::connectedComponents(con, labels, 4, CV_32S);

    return labels;
}






#endif /* CHECKOUT_PLUGIN_CORE_H_ */
