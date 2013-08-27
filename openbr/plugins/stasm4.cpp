#include <stasm_lib.h>
#include <stasmcascadeclassifier.h>
#include <opencv2/opencv.hpp>
#include "openbr_internal.h"
#include "openbr/core/qtutils.h"
#include "openbr/core/opencvutils.h"
#include <QString>
#include <Eigen/SVD>

using namespace std;
using namespace cv;

namespace br
{

class StasmResourceMaker : public ResourceMaker<StasmCascadeClassifier>
{
private:
    StasmCascadeClassifier *make() const
    {
        StasmCascadeClassifier *stasmCascade = new StasmCascadeClassifier();
        if (!stasmCascade->load(Globals->sdkPath.toStdString() + "/share/openbr/models/"))
            qFatal("Failed to load Stasm Cascade");
        return stasmCascade;
    }
};

/*!
 * \ingroup initializers
 * \brief Initialize Stasm
 * \author Scott Klum \cite sklum
 */
class StasmInitializer : public Initializer
{
    Q_OBJECT

    void initialize() const
    {
        Globals->abbreviations.insert("RectFromStasmEyes","RectFromPoints([29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47],0.125,6.0)");
        Globals->abbreviations.insert("RectFromStasmBrow","RectFromPoints([16,17,18,19,20,21,22,23,24,25,26,27],0.15,5)");
        Globals->abbreviations.insert("RectFromStasmNose","RectFromPoints([48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58],0.15,1.25)");
        Globals->abbreviations.insert("RectFromStasmMouth","RectFromPoints([59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76],0.3,2.5)");
    }
};

BR_REGISTER(Initializer, StasmInitializer)

/*!
 * \ingroup transforms
 * \brief Wraps STASM key point detector
 * \author Scott Klum \cite sklum
 */
class StasmTransform : public UntrainableTransform
{
    Q_OBJECT

    Q_PROPERTY(bool stasm3Format READ get_stasm3Format WRITE set_stasm3Format RESET reset_stasm3Format STORED false)
    BR_PROPERTY(bool, stasm3Format, false)
    Q_PROPERTY(bool clearLandmarks READ get_clearLandmarks WRITE set_clearLandmarks RESET reset_clearLandmarks STORED false)
    BR_PROPERTY(bool, clearLandmarks, false)

    Resource<StasmCascadeClassifier> stasmCascadeResource;

    void init()
    {
        if (!stasm_init(qPrintable(Globals->sdkPath + "/share/openbr/models/stasm"), 0)) qFatal("Failed to initalize stasm.");
        stasmCascadeResource.setResourceMaker(new StasmResourceMaker());
    }

    void project(const Template &src, Template &dst) const
    {
        if (src.m().channels() != 1) qFatal("Stasm expects single channel matrices.");

        dst = src;

        StasmCascadeClassifier *stasmCascade = stasmCascadeResource.acquire();

        int foundface;
        int nLandmarks = stasm_NLANDMARKS;
        float landmarks[2 * stasm_NLANDMARKS];
        stasm_search_single(&foundface, landmarks, reinterpret_cast<const char*>(src.m().data), src.m().cols, src.m().rows, *stasmCascade, NULL, NULL);

        if (stasm3Format) {
            nLandmarks = 76;
            stasm_convert_shape(landmarks, nLandmarks);
        }

        stasmCascadeResource.release(stasmCascade);

        if (clearLandmarks) {
            dst.file.clearPoints();
            dst.file.clearRects();
        }

        if (!foundface) {
            qWarning("No face found in %s", qPrintable(src.file.fileName()));
        } else {
            for (int i = 0; i < nLandmarks; i++) {
                QPointF point(landmarks[2 * i], landmarks[2 * i + 1]);
                dst.file.appendPoint(point);
                if (i == 38) dst.file.set("StasmRightEye",point);
                if (i == 39) dst.file.set("StasmLeftEye", point);
            }
        }

        dst.m() = src.m();
    }
};

BR_REGISTER(Transform, StasmTransform)

} // namespace br

#include "stasm4.moc"
