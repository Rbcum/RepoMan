#ifndef DIFFUTILS_H
#define DIFFUTILS_H

#include <QStringList>

struct DiffHunk
{
    int oldStart;
    int oldTotal;
    int newStart;
    int newTotal;

    QList<int> unifiedOldLNs;
    QList<int> unifiedNewLNs;
    QList<int> splitOldLNs;
    QList<int> splitNewLNs;

    QStringList lines[3];
};

enum DiffMode
{
    Unified = 0,
    SplitOld,
    SplitNew
};

QList<DiffHunk> parseDiffHunks(const QString &diffText);

void findTargetHunk(
    DiffMode mode, const QList<DiffHunk> hunks, int blockNumber, int &hunkIndex, int &lineIndex);

#endif  // DIFFUTILS_H
