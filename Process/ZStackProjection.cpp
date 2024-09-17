#include "processingspeedtesting.h"
#include <201607/AlgorithmCommon.h>


ProcessingSpeedTesting::ProcessingSpeedTesting()
{
    description("Tools/Speed/Speed Testing",
                QStringList() << "wiest.daessle@gmail.com",
                "Performance test : network load and computation simulator");

    use(&image, "input-image", "Input Image")
        .noImageAutoLoading()
        .channelsAsVector() // This process does handle all the channels together,
        .withProperties(QStringList() << "X" << "Y" << "HorizontalPixelDimension" << "VerticalPixelDimension")

        ;

    use(&option, QStringList() << "Compute Only" << "Load Only" << "Load and Compute", "Load and Compute", "Speed test & load generator")
            .setDefault(0)
            ;
    use(&duration, "duration", "Duration of the process (s)")
        .setRange(1, 360)
        .setDefault(30)
        ;

    use(&plugin_semaphore, "plugin_semaphore", "Use per run semaphore, instead of global one")
        .setDefault(false)
        .setAsBool()
        ;



    produces(&time, "Time", "Processing time")
            .setAggregationType(RegistrableParent::Mean);
//    produces(&this->tile_avg_time, "Tiled_time", "Processing time for Tiled data");

    produces(&pixels, "pixels", "Count the number of pixels")
        .setAggregationType(RegistrableParent::Sum);

}

CheckoutProcessPluginInterface* ProcessingSpeedTesting::clone()
{
    return new ProcessingSpeedTesting();
}

void ProcessingSpeedTesting::exec()
{
    time = 0;

    try
    {
        QSemaphore& semaphore = PhenoLinkImage::getSemaphore();

       // int t = rint(sqrt(image.count()));
        std::chrono::high_resolution_clock::time_point t1, t2;
        t1 = t2 = std::chrono::high_resolution_clock::now();
        if (option == 1 || option == 2)
        {
            int channelNb = (int)image.getChannelCount();

            if (plugin_semaphore) semaphore.acquire();

            for (int chann = 0; chann < channelNb; ++chann)
                for (unsigned i = 0; i < image.count(); ++i)
                {
                    int count = image.getImage(i, chann, QString(), !plugin_semaphore).total();
                    if (count == 0)
                        qDebug() << image.getImageFile(i, chann) << "not loaded";
                    pixels += count;
                }

            if (plugin_semaphore) semaphore.release();

            t2 = std::chrono::high_resolution_clock::now();
            time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        }


        if (option == 0 || option == 2)
        {
            t1 = t2 = std::chrono::high_resolution_clock::now();

            std::vector<double> bigvec(100000);

            while (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() < 1000 * duration )
            {
                for (unsigned i = 0; i < bigvec.size()-1; ++i)
                    bigvec[i] *= bigvec[i]+bigvec[i+1];
                t2 = std::chrono::high_resolution_clock::now();
            }
          time += std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        }

    }
    catch (cv::Exception& ex)
    {
        qDebug() << ex.what();
    }



}
