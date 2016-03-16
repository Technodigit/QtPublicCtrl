/****************************************************************************
** 
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
** 
** This file is part of a Qt Solutions component.
**
** Commercial Usage  
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Solutions Commercial License Agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and Nokia.
** 
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
** 
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.1, included in the file LGPL_EXCEPTION.txt in this
** package.
** 
** GNU General Public License Usage 
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
** 
** Please note Third Party Software included with Qt Solutions may impose
** additional restrictions and it is the user's responsibility to ensure
** that they have met the licensing requirements of the GPL, LGPL, or Qt
** Solutions Commercial license and the relevant license of the Third
** Party Software they are using.
** 
** If you are unsure which license is appropriate for your use, please
** contact Nokia at qt-info@nokia.com.
** 
****************************************************************************/
// modification 24/06/15
// - move all the class in the header qtcolorpicker.h
// - remove text and icon inside the button
// - add stylesheet to the button to use the current color

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtGui\QPainter>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolTip>
#include <QtGui/QFocusEvent>
#include <math.h>

#include "qtcolorpicker.h"

/*! \class QtColorPicker

\brief The QtColorPicker class provides a widget for selecting
colors from a popup color grid.

Users can invoke the color picker by clicking on it, or by
navigating to it and pressing Space. They can use the mouse or
arrow keys to navigate between colors on the grid, and select a
color by clicking or by pressing Enter or Space. The
colorChanged() signal is emitted whenever the color picker's color
changes.

The widget also supports negative selection: Users can click and
hold the mouse button on the QtColorPicker widget, then move the
mouse over the color grid and release the mouse button over the
color they wish to select.

The color grid shows a customized selection of colors. An optional
ellipsis "..." button (signifying "more") can be added at the
bottom of the grid; if the user presses this, a QColorDialog pops
up and lets them choose any color they like. This button is made
available by using setColorDialogEnabled().

When a color is selected, the QtColorPicker widget shows the color
and its name. If the name cannot be determined, the translatable
name "Custom" is used.

The QtColorPicker object is optionally initialized with the number
of columns in the color grid. Colors are then added left to right,
top to bottom using insertColor(). If the number of columns is not
set, QtColorPicker calculates the number of columns and rows that
will make the grid as square as possible.

\code
DrawWidget::DrawWidget(QWidget *parent, const char *name)
{
QtColorPicker *picker = new QtColorPicker(this);
picker->insertColor(red, "Red"));
picker->insertColor(QColor("green"), "Green"));
picker->insertColor(QColor(0, 0, 255), "Blue"));
picker->insertColor(white);

connect(colors, SIGNAL(colorChanged(const QColor &)), SLOT(setCurrentColor(const QColor &)));
}
\endcode

An alternative to adding colors manually is to initialize the grid
with QColorDialog's standard colors using setStandardColors().

QtColorPicker also provides a the static function getColor(),
which pops up the grid of standard colors at any given point.

\img colorpicker1.png
\img colorpicker2.png

\sa QColorDialog
*/

/*! \fn QtColorPicker::colorChanged(const QColor &color)

This signal is emitted when the QtColorPicker's color is changed.
\a color is the new color.

To obtain the color's name, use text().
*/

/*!
Constructs a QtColorPicker widget. The popup will display a grid
with \a cols columns, or if \a cols is -1, the number of columns
will be calculated automatically.

If \a enableColorDialog is true, the popup will also have a "More"
button (signified by an ellipsis "...") that presents a
QColorDialog when clicked.

After constructing a QtColorPicker, call insertColor() to add
individual colors to the popup grid, or call setStandardColors()
to add all the standard colors in one go.

The \a parent argument is passed to QFrame's constructor.

\sa QFrame
*/
QtColorPicker::QtColorPicker(QWidget *parent,
							 int cols, bool enableColorDialog)
							 : QPushButton(parent), popup(0), withColorDialog(enableColorDialog)
{
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	setAutoDefault(false);
	setAutoFillBackground(true);
	setCheckable(true);

	// Set text
	//setText(tr("Black"));
	firstInserted = false;

	// Create and set icon
	col = Qt::black;
	dirty = true;

	// Create color grid popup and connect to it.
	popup = new ColorPickerPopup(cols, withColorDialog, this);
	connect(popup, SIGNAL(selected(const QColor &)),
		SLOT(setCurrentColor(const QColor &)));
	connect(popup, SIGNAL(hid()), SLOT(popupClosed()));

	// Connect this push button's pressed() signal.
	connect(this, SIGNAL(toggled(bool)), SLOT(buttonPressed(bool)));
}

/*!
Destructs the QtColorPicker.
*/
QtColorPicker::~QtColorPicker()
{
}

/*! \internal

Pops up the color grid, and makes sure the status of
QtColorPicker's button is right.
*/
void QtColorPicker::buttonPressed(bool toggled)
{
	if (!toggled)
		return;

	const QRect desktop = QApplication::desktop()->geometry();
	// Make sure the popup is inside the desktop.
	QPoint pos = mapToGlobal(rect().bottomLeft());
	if (pos.x() < desktop.left())
		pos.setX(desktop.left());
	if (pos.y() < desktop.top())
		pos.setY(desktop.top());

	if ((pos.x() + popup->sizeHint().width()) > desktop.width())
		pos.setX(desktop.width() - popup->sizeHint().width());
	if ((pos.y() + popup->sizeHint().height()) > desktop.bottom())
		pos.setY(desktop.bottom() - popup->sizeHint().height());
	popup->move(pos);

	if (ColorPickerItem *item = popup->find(col))
		item->setSelected(true);

	// Remove focus from this widget, preventing the focus rect
	// from showing when the popup is shown. Order an update to
	// make sure the focus rect is cleared.
	clearFocus();
	update();

	// Allow keyboard navigation as soon as the popup shows.
	popup->setFocus();

	// Execute the popup. The popup will enter the event loop.
	popup->show();
}

/*!
\internal
*/
void QtColorPicker::paintEvent(QPaintEvent *e)
{
	if (dirty) 
	{
		int hue, saturation, luminance, alpha;
		col.getHsv(&hue, &saturation, &luminance, &alpha);
		QColor topColor, topColor2, botColor;
		topColor.setHsv(hue, qMax(saturation-70, 0), qMax(luminance-40, 0), alpha);
		topColor2.setHsv(hue, qMax(saturation-5, 0), qMax(luminance-10, 0), alpha);
		botColor.setHsv(hue, qMin(saturation+30, 255), qMin(luminance+20, 255), alpha);

		int tmpGray = qMin((int)(qGray(topColor.rgb())*1.4), 255);
		QColor topGray( tmpGray, tmpGray, tmpGray );
		tmpGray = qMin((int)(qGray(topColor2.rgb())*1.4), 255);
		QColor topGray2( tmpGray, tmpGray, tmpGray );
		tmpGray = qMin((int)(qGray(col.rgb())*1.4), 255);
		QColor theGray( tmpGray, tmpGray, tmpGray );
		tmpGray = qMin((int)(qGray(botColor.rgb())*1.4), 255);
		QColor botGray( tmpGray, tmpGray, tmpGray );
		setStyleSheet(
			QString("QtColorPicker {"
			"background-color: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 %1, stop: 0.05 %2, stop: 0.5 %3, stop: 1 %4);"
			"border-width: 1px;"
			"border-color: #5c5c5c;"
			"border-style: solid;"
			"border-radius: 2px;"
			"}"

			"QtColorPicker:hover {"
			"background-color: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 %4, stop: 0.05 %3, stop: 0.95 %2, stop: 1 %1);"
			"border-color: #7EB4EA;"
			"}"

			"QtColorPicker:pressed {"
			"background-color: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 %1, stop: 0.05 %2, stop: 0.7 %3, stop: 1 %4);"
			"}"

			"QtColorPicker:disabled {"
			"background-color: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 %5, stop: 0.05 %6, stop: 0.5 %7, stop: 1 %8);"
			"}"
			).arg(topColor.name()).arg(topColor2.name()).arg(col.name()).arg(botColor.name())
			.arg(topGray.name()).arg(topGray2.name()).arg(theGray.name()).arg(botGray.name())
			);
		dirty = false;
	}

	QPushButton::paintEvent(e);

}

/*! \internal

Makes sure the button isn't pressed when the popup hides.
*/
void QtColorPicker::popupClosed()
{
	setChecked(false);
	setFocus();
}

/*!
Returns the currently selected color.

\sa text()
*/
QColor QtColorPicker::currentColor() const
{
	return col;
}

/*!
Returns the color at position \a index.
*/
QColor QtColorPicker::color(int index) const
{
	return popup->color(index);
}

/*!
Adds the 17 predefined colors from the Qt namespace.

(The names given to the colors, "Black", "White", "Red", etc., are
all translatable.)

\sa insertColor()
*/
void QtColorPicker::setStandardColors()
{
	insertColor(Qt::black, tr("Black"));
	insertColor(Qt::white, tr("White"));
	insertColor(Qt::red, tr("Red"));
	insertColor(Qt::darkRed, tr("Dark red"));
	insertColor(Qt::green, tr("Green"));
	insertColor(Qt::darkGreen, tr("Dark green"));
	insertColor(Qt::blue, tr("Blue"));
	insertColor(Qt::darkBlue, tr("Dark blue"));
	insertColor(Qt::cyan, tr("Cyan"));
	insertColor(Qt::darkCyan, tr("Dark cyan"));
	insertColor(Qt::magenta, tr("Magenta"));
	insertColor(Qt::darkMagenta, tr("Dark magenta"));
	insertColor(Qt::yellow, tr("Yellow"));
	insertColor(Qt::darkYellow, tr("Dark yellow"));
	insertColor(Qt::gray, tr("Gray"));
	insertColor(Qt::darkGray, tr("Dark gray"));
	insertColor(Qt::lightGray, tr("Light gray"));
}
void QtColorPicker::setColorsWithoutText()
{
	insertColor(Qt::black, tr(""));
	insertColor(Qt::gray, tr(""));
	insertColor(Qt::white, tr(""));
	insertColor(Qt::red, tr(""));
	insertColor(Qt::green, tr(""));
	insertColor(Qt::blue, tr(""));
	insertColor(Qt::magenta, tr(""));
}


/*!
Makes \a color current. If \a color is not already in the color grid, it
is inserted with the text "Custom".

This function emits the colorChanged() signal if the new color is
valid, and different from the old one.
*/
void QtColorPicker::setCurrentColor(const QColor &color,  bool isText)
{
	if (col == color || !color.isValid())
		return;

	ColorPickerItem *item = popup->find(color);
	if (!item) 
	{
		//if(isText)
		//	insertColor(color, tr("Custom"));
		//else
		insertColor(color, tr(""));
		item = popup->find(color);
	}

	col = color;
	//setText(item->text());

	dirty = true;

	popup->hide();
	repaint();

	if(item)
		item->setSelected(true);

	emit colorChanged(color);
}

/*!
Adds the color \a color with the name \a text to the color grid,
at position \a index. If index is -1, the color is assigned
automatically assigned a position, starting from left to right,
top to bottom.
*/
void QtColorPicker::insertColor(const QColor &color, const QString &text, int index)
{
	popup->insertColor(color, text, index);
	if (!firstInserted) 
	{
		col = color;
		//setText(text);
		firstInserted = true;
	}
}

/*! \property QtColorPicker::colorDialog
\brief Whether the ellipsis "..." (more) button is available.

If this property is set to TRUE, the color grid popup will include
a "More" button (signified by an ellipsis, "...") which pops up a
QColorDialog when clicked. The user will then be able to select
any custom color they like.
*/
void QtColorPicker::setColorDialogEnabled(bool enabled)
{
	withColorDialog = enabled;
}
bool QtColorPicker::colorDialogEnabled() const
{
	return withColorDialog;
}

/*!
Pops up a color grid with Qt default colors at \a point, using
global coordinates. If \a allowCustomColors is true, there will
also be a button on the popup that invokes QColorDialog.

For example:

\code
void Drawer::mouseReleaseEvent(QMouseEvent *e)
{
if (e->button() & RightButton) {
QColor color = QtColorPicker::getColor(mapToGlobal(e->pos()));
}
}
\endcode
*/
QColor QtColorPicker::getColor(const QPoint &point, bool allowCustomColors)
{
	ColorPickerPopup popup(-1, allowCustomColors);

	popup.insertColor(Qt::black, tr("Black"), 0);
	popup.insertColor(Qt::white, tr("White"), 1);
	popup.insertColor(Qt::red, tr("Red"), 2);
	popup.insertColor(Qt::darkRed, tr("Dark red"), 3);
	popup.insertColor(Qt::green, tr("Green"), 4);
	popup.insertColor(Qt::darkGreen, tr("Dark green"), 5);
	popup.insertColor(Qt::blue, tr("Blue"), 6);
	popup.insertColor(Qt::darkBlue, tr("Dark blue"), 7);
	popup.insertColor(Qt::cyan, tr("Cyan"), 8);
	popup.insertColor(Qt::darkCyan, tr("Dark cyan"), 9);
	popup.insertColor(Qt::magenta, tr("Magenta"), 10);
	popup.insertColor(Qt::darkMagenta, tr("Dark magenta"), 11);
	popup.insertColor(Qt::yellow, tr("Yellow"), 12);
	popup.insertColor(Qt::darkYellow, tr("Dark yellow"), 13);
	popup.insertColor(Qt::gray, tr("Gray"), 14);
	popup.insertColor(Qt::darkGray, tr("Dark gray"), 15);
	popup.insertColor(Qt::lightGray, tr("Light gray"), 16);

	popup.move(point);
	popup.exec();
	return popup.lastSelected();
}

/*! \internal

Constructs the popup widget.
*/
ColorPickerPopup::ColorPickerPopup(int width, bool withColorDialog,
								   QWidget *parent,
								   Qt::WindowFlags f,
								   bool iWithAlphaChannel)
								   : QFrame(parent, f),
								   isPopup(true),
								   withAlpha(iWithAlphaChannel)
{
	if( f == Qt::Widget)
	{
		isPopup = false;
		setFocusPolicy(Qt::NoFocus);
		setFrameStyle(QFrame::NoFrame);
	}
	else
	{
		setFocusPolicy(Qt::StrongFocus);
		setFrameStyle(QFrame::StyledPanel);
		setStyleSheet(
			QString(
			"border-width: 1px;"
			"border-color: #5c5c5c;"
			"border-style: solid;"
			"border-radius: 5px;"
			));
	}

	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	setMouseTracking(true);
	cols = width;

	if (withColorDialog) 
	{
		moreButton = new ColorPickerButton(this);
		connect(moreButton, SIGNAL(clicked()), SLOT(getColorFromDialog()));
	}
	else 
	{
		moreButton = 0;
	}

	eventLoop = 0;
	grid = 0;
	regenerateGrid();

}


/*! \internal

Destructs the popup widget.
*/
ColorPickerPopup::~ColorPickerPopup()
{
	if (eventLoop)
		eventLoop->exit();
}

/*! \internal

If there is an item whole color is equal to \a col, returns a
pointer to this item; otherwise returns 0.
*/
ColorPickerItem *ColorPickerPopup::find(const QColor &col) const
{
	for (int i = 0; i < items.size(); ++i) {
		if (items.at(i) && items.at(i)->color() == col)
			return items.at(i);
	}

	return 0;
}

/*! \internal

Adds \a item to the grid. The items are added from top-left to
bottom-right.
*/
void ColorPickerPopup::insertColor(const QColor &col, const QString &text, int index)
{
	// Don't add colors that we have already.
	ColorPickerItem *existingItem = find(col);
	ColorPickerItem *lastSelectedItem = find(lastSelected());

	if (existingItem) {
		if (lastSelectedItem && existingItem != lastSelectedItem)
			lastSelectedItem->setSelected(false);
		existingItem->setFocus();
		existingItem->setSelected(true);
		return;
	}

	ColorPickerItem *item = new ColorPickerItem(col, text, this);

	if (lastSelectedItem) {
		lastSelectedItem->setSelected(false);
	}
	else {
		item->setSelected(true);
		lastSel = col;
	}
	item->setFocus();

	connect(item, SIGNAL(selected()), SLOT(updateSelected()));

	if (index == -1)
		index = items.count();

	items.insert((unsigned int)index, item);
	regenerateGrid();

	update();
}

/*! \internal

*/
QColor ColorPickerPopup::color(int index) const
{
	if (index < 0 || index > (int) items.count() - 1)
		return QColor();

	ColorPickerPopup *that = (ColorPickerPopup *)this;
	return that->items.at(index)->color();
}

/*! \internal

*/
void ColorPickerPopup::exec()
{
	show();

	QEventLoop e;
	eventLoop = &e;
	(void) e.exec();
	eventLoop = 0;
}

/*! \internal

*/
void ColorPickerPopup::updateSelected()
{
	QLayoutItem *layoutItem;
	int i = 0;
	while ((layoutItem = grid->itemAt(i)) != 0) {
		QWidget *w = layoutItem->widget();
		if (w && w->inherits("ColorPickerItem")) {
			ColorPickerItem *litem = reinterpret_cast<ColorPickerItem *>(layoutItem->widget());
			if (litem != sender())
				litem->setSelected(false);
		}
		++i;
	}

	if (sender() && sender()->inherits("ColorPickerItem")) {
		ColorPickerItem *item = (ColorPickerItem *)sender();
		lastSel = item->color();
		emit selected(item->color());
	}

	if( isPopup)
		hide();
}

/*! \internal

*/
void ColorPickerPopup::mouseReleaseEvent(QMouseEvent *e)
{
	if (!rect().contains(e->pos()) && isPopup)
		hide();
}

/*! \internal

Controls keyboard navigation and selection on the color grid.
*/
void ColorPickerPopup::keyPressEvent(QKeyEvent *e)
{
	int curRow = 0;
	int curCol = 0;

	bool foundFocus = false;
	for (int j = 0; !foundFocus && j < grid->rowCount(); ++j) {
		for (int i = 0; !foundFocus && i < grid->columnCount(); ++i) {
			if (widgetAt[j][i] && widgetAt[j][i]->hasFocus()) {
				curRow = j;
				curCol = i;
				foundFocus = true;
				break;
			}
		}
	}

	switch (e->key()) {
	case Qt::Key_Left:
		if (curCol > 0) --curCol;
		else if (curRow > 0) { --curRow; curCol = grid->columnCount() - 1; }
		break;
	case Qt::Key_Right:
		if (curCol < grid->columnCount() - 1 && widgetAt[curRow][curCol + 1]) ++curCol;
		else if (curRow < grid->rowCount() - 1) { ++curRow; curCol = 0; }
		break;
	case Qt::Key_Up:
		if (curRow > 0) --curRow;
		else curCol = 0;
		break;
	case Qt::Key_Down:
		if (curRow < grid->rowCount() - 1) {
			QWidget *w = widgetAt[curRow + 1][curCol];
			if (w) {
				++curRow;
			} else for (int i = 1; i < grid->columnCount(); ++i) {
				if (!widgetAt[curRow + 1][i]) {
					curCol = i - 1;
					++curRow;
					break;
				}
			}
		}
		break;
	case Qt::Key_Space:
	case Qt::Key_Return:
	case Qt::Key_Enter: {
		QWidget *w = widgetAt[curRow][curCol];
		if (w && w->inherits("ColorPickerItem")) {
			ColorPickerItem *wi = reinterpret_cast<ColorPickerItem *>(w);
			wi->setSelected(true);

			QLayoutItem *layoutItem;
			int i = 0;
			while ((layoutItem = grid->itemAt(i)) != 0) {
				QWidget *w = layoutItem->widget();
				if (w && w->inherits("ColorPickerItem")) {
					ColorPickerItem *litem
						= reinterpret_cast<ColorPickerItem *>(layoutItem->widget());
					if (litem != wi)
						litem->setSelected(false);
				}
				++i;
			}

			lastSel = wi->color();
			emit selected(wi->color());
			if(isPopup)
				hide();
		} else if (w && w->inherits("QPushButton")) {
			ColorPickerItem *wi = reinterpret_cast<ColorPickerItem *>(w);
			wi->setSelected(true);

			QLayoutItem *layoutItem;
			int i = 0;
			while ((layoutItem = grid->itemAt(i)) != 0) {
				QWidget *w = layoutItem->widget();
				if (w && w->inherits("ColorPickerItem")) {
					ColorPickerItem *litem
						= reinterpret_cast<ColorPickerItem *>(layoutItem->widget());
					if (litem != wi)
						litem->setSelected(false);
				}
				++i;
			}

			lastSel = wi->color();
			emit selected(wi->color());
			if(isPopup)	
				hide();
		}
						}
						break;
	case Qt::Key_Escape:
		{
			if(isPopup)	
				hide();
		}
		break;
	default:
		e->ignore();
		break;
	}

	widgetAt[curRow][curCol]->setFocus();
}

/*! \internal

*/
void ColorPickerPopup::hideEvent(QHideEvent *e)
{
	if (eventLoop) {
		eventLoop->exit();
	}

	setFocus();

	emit hid();
	QFrame::hideEvent(e);
}

/*! \internal

*/
QColor ColorPickerPopup::lastSelected() const
{
	return lastSel;
}

/*! \internal

Sets focus on the popup to enable keyboard navigation. Draws
focusRect and selection rect.
*/
void ColorPickerPopup::showEvent(QShowEvent *)
{
	bool foundSelected = false;
	for (int i = 0; i < grid->columnCount(); ++i) {
		for (int j = 0; j < grid->rowCount(); ++j) {
			QWidget *w = widgetAt[j][i];
			if (w && w->inherits("ColorPickerItem")) {
				if (((ColorPickerItem *)w)->isSelected()) {
					w->setFocus();
					foundSelected = true;
					break;
				}
			}
		}
	}

	if (!foundSelected) {
		if (items.count() == 0)
			setFocus();
		else
			widgetAt[0][0]->setFocus();
	}
}

/*!

*/
void ColorPickerPopup::regenerateGrid()
{
	widgetAt.clear();

	int columns = cols;
	if (columns == -1)
		columns = (int) ceil(sqrt((float) items.count()));

	// When the number of columns grows, the number of rows will
	// fall. There's no way to shrink a grid, so we create a new
	// one.
	if (grid) delete grid;
	grid = new QGridLayout(this);
	grid->setMargin(5);
	grid->setSpacing(1);

	int ccol = 0, crow = 0;
	for (int i = 0; i < items.size(); ++i) {
		if (items.at(i)) {
			widgetAt[crow][ccol] = items.at(i);
			grid->addWidget(items.at(i), crow, ccol++);
			if (ccol == columns) {
				++crow;
				ccol = 0;
			}
		}
	}

	if (moreButton) {
		grid->addWidget(moreButton, crow, ccol);
		widgetAt[crow][ccol] = moreButton;
	}
	updateGeometry();
}

/*! \internal

Copies the color dialog's currently selected item and emits
itemSelected().
*/
void ColorPickerPopup::getColorFromDialog()
{
	QColorDialog::ColorDialogOptions options = 0;
	if( withAlpha)
		options |= QColorDialog::ShowAlphaChannel;

	QColor col = QColorDialog::getColor(lastSel, parentWidget(), QString(), options);
	if (!col.isValid())
		return;

	insertColor(col, tr("Custom"), -1);
	lastSel = col;
	emit selected(col);
}

/*!
Constructs a ColorPickerItem whose color is set to \a color, and
whose name is set to \a text.
*/
ColorPickerItem::ColorPickerItem(const QColor &color, const QString &text,
								 QWidget *parent)
								 : QToolButton(parent), c(color), t(text), sel(false)
{
	setToolTip(t);
	SetStyleSheet(c);
	setFixedHeight(22);
	setFixedWidth(22);
	setObjectName("ColorPickerItem");
}

/*!
Destructs a ColorPickerItem.
*/
ColorPickerItem::~ColorPickerItem()
{
}

/*!
Returns the item's color.

\sa text()
*/
QColor ColorPickerItem::color() const
{
	return c;
}

/*!

*/
bool ColorPickerItem::isSelected() const
{
	return sel;
}

/*!

*/
void ColorPickerItem::setSelected(bool selected)
{
	sel = selected;
	update();
}

/*!
Sets the item's color to \a color, and its name to \a text.
*/
void ColorPickerItem::setColor(const QColor &color, const QString &text)
{
	c = color;
	t = text;
	setToolTip(t);
	update();

	SetStyleSheet(c);
}

void ColorPickerItem::SetStyleSheet(const QColor& iColor)
{
	setStyleSheet(
		QString("ColorPickerItem {"
		"border-width: 1px;"
		"border-color: #5c5c5c;"
		"border-style: solid;"
		"border-radius: 2px;"
		"background-color: %1"
		"}"

		"ColorPickerItem:hover {"
		"border-color: #7EB4EA;"
		"}"

		"ColorPickerItem:focus {"
		"border-color: #000000;"
		"border-radius: 1px;"
		"}"
		).arg(iColor.name()));
}

/*!

*/
void ColorPickerItem::mouseReleaseEvent(QMouseEvent *event)
{
	__super::mouseReleaseEvent(event);
	sel = true;
	emit selected();
}

/*!

*/
ColorPickerButton::ColorPickerButton(QWidget *parent)
	: QToolButton(parent)
{
	setStyleSheet(
		QString("ColorPickerButton {"
		"border-width: 1px;"
		"border-color: #5c5c5c;"
		"border-style: solid;"
		"border-radius: 2px;"
		"}"

		"ColorPickerButton:hover {"
		"border-color: #7EB4EA;"
		"}"
		));
	setText("...");
	setFixedHeight(22);
	setFixedWidth(22);
}


