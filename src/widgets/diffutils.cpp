#include "diffutils.h"

#include <QRegularExpression>

QList<DiffHunk> parseDiffHunks(const QString &diffText)
{
    static QRegularExpression splitRE("\r?\n");
    QStringList lines = diffText.trimmed().split(splitRE);

    QList<DiffHunk> hunks;
    std::optional<DiffHunk> hk;

    int n = 0;
    while (n < lines.size() && !lines[n].startsWith("@@")) {
        n++;
    }

    int oldLN = 0;
    int newLN = 0;
    int addCount = 0;
    int deleteCount = 0;
    for (int i = n; i < lines.size(); i++) {
        QString line = lines[i];
        if (line.startsWith("@@")) {
            if (hk) {
                hunks.append(hk.value());
            }
            hk = DiffHunk();
            QStringList nums = line.split(" ");
            QStringList oldNums = nums[1].mid(1).split(",");
            QStringList newNums = nums[2].mid(1).split(",");
            hk->oldStart = oldLN = oldNums[0].toInt();
            hk->oldTotal = oldNums.last().toInt();
            hk->newStart = newLN = newNums[0].toInt();
            hk->newTotal = newNums.last().toInt();

            hk->unifiedOldLNs.append(0);
            hk->unifiedNewLNs.append(0);

            hk->splitOldLNs.append(0);
            hk->lines[SplitOld].append(line);
            hk->splitNewLNs.append(0);
            hk->lines[SplitNew].append(line);
        } else if (line.startsWith("+")) {
            hk->unifiedOldLNs.append(0);
            hk->unifiedNewLNs.append(newLN);

            hk->splitNewLNs.append(newLN++);
            hk->lines[SplitNew].append(line);
            addCount++;
        } else if (line.startsWith("-")) {
            hk->unifiedOldLNs.append(oldLN);
            hk->unifiedNewLNs.append(0);

            hk->splitOldLNs.append(oldLN++);
            hk->lines[SplitOld].append(line);
            deleteCount++;
        } else {
            if (addCount || deleteCount) {
                int delta = qAbs(addCount - deleteCount);
                if (addCount > deleteCount) {
                    for (int i = 0; i < delta; ++i) {
                        hk->splitOldLNs.append(0);
                        hk->lines[SplitOld].append("");
                    }
                } else {
                    for (int i = 0; i < delta; ++i) {
                        hk->splitNewLNs.append(0);
                        hk->lines[SplitNew].append("");
                    }
                }
                addCount = 0;
                deleteCount = 0;
            }

            hk->unifiedOldLNs.append(oldLN);
            hk->unifiedNewLNs.append(newLN);

            hk->splitOldLNs.append(oldLN++);
            hk->lines[SplitOld].append(line);
            hk->splitNewLNs.append(newLN++);
            hk->lines[SplitNew].append(line);
        }
        hk->lines[Unified].append(line);
    }
    if (hk) {
        hunks.append(hk.value());
    }
    return hunks;
}

void findTargetHunk(
    DiffMode mode, const QList<DiffHunk> hunks, int blockNumber, int &hunkIndex, int &lineIndex)
{
    for (int i = 0; i < hunks.size(); ++i) {
        const QStringList &lines = hunks[i].lines[mode];
        if (lines.size() > blockNumber) {
            hunkIndex = i;
            lineIndex = blockNumber;
            return;
        }
        blockNumber -= lines.size();
    }
}
