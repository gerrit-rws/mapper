/*
 *    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "map_printer.h"

#include <limits>

#include <QPainter>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "../util.h"
#include "../map.h"
#include "../settings.h"
#include "../util/xml_stream_util.h"

// #### MapPrinterPageFormat ###

MapPrinterPageFormat::MapPrinterPageFormat(QSizeF page_rect_size, qreal margin) 
 : paper_size(QPrinter::Custom),
   orientation(QPrinter::Portrait),
   page_rect(QRectF(QPointF(margin, margin), page_rect_size)),
   h_overlap(5.0), // mm
   v_overlap(5.0)  // mm
{
	const qreal double_margin = 2 * margin;
	paper_dimensions = page_rect.size() + QSizeF(double_margin, double_margin);
}



// ### MapPrinterOptions ###

MapPrinterOptions::MapPrinterOptions(unsigned int scale, unsigned int resolution)
 : scale(scale),
   resolution(resolution),
   show_templates(false),
   show_grid(false),
   simulate_overprinting(false)
{
	// nothing
}



// ### MapPrinterConfig ###

MapPrinterConfig::MapPrinterConfig(const Map& map)
 : printer_name("DEFAULT"),
   print_area(map.calculateExtent()),
   page_format(),                      // Use defaults.
   options(map.getScaleDenominator()),
   center_print_area(false),
   single_page_print_area(false)
{
	options.resolution = 600.0;
	options.scale = map.getScaleDenominator();
	options.show_grid = false;
	options.show_templates = false;
	options.simulate_overprinting = false;
}

MapPrinterConfig::MapPrinterConfig(const Map& map, QXmlStreamReader& xml)
 : printer_name("DEFAULT"),
   print_area(0.0, 0.0, 100.0, 100.0), // Avoid expensive calculation before loading.
   page_format(),                      // Use defaults.
   options(map.getScaleDenominator()),
   center_print_area(false),
   single_page_print_area(false)
{
	ScopedXmlReaderElement printer_config_element(xml);
	
	printer_config_element.readAttribute("scale", options.scale);
	printer_config_element.readAttribute("resolution", options.resolution);
	printer_config_element.readAttribute("templates_visible", options.show_templates);
	printer_config_element.readAttribute("grid_visible", options.show_grid);
	printer_config_element.readAttribute("simulate_overprinting", options.simulate_overprinting);
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == "page_format")
		{
			ScopedXmlReaderElement page_format_element(xml);
			QString value;
			page_format_element.readAttribute("paper_size", value);
			const QHash< int, const char* >& paper_size_names = MapPrinter::paperSizeNames();
			for (int i = 0; i < paper_size_names.count(); ++i)
			{
				if (value == paper_size_names[i])
					page_format.paper_size = (QPrinter::PaperSize)i;
			}
			page_format_element.readAttribute("orientation", value);
			page_format.orientation =
			  (value == "portrait") ? QPrinter::Portrait : QPrinter::Landscape;
			while (xml.readNextStartElement())
			{
				if (xml.name() == "paper_dimensions")
				{
					ScopedXmlReaderElement(xml).read(page_format.paper_dimensions);
				}
				else if (xml.name() == "page_rect")
				{
					ScopedXmlReaderElement(xml).read(page_format.page_rect);
				}
				else
					xml.skipCurrentElement();
			}
		}
		else if (xml.name() == "print_area")
		{
			ScopedXmlReaderElement print_area_element(xml);
			print_area_element.read(print_area);
			print_area_element.readAttribute("center_area", center_print_area);
			print_area_element.readAttribute("single_page_print_area", single_page_print_area);
		}
		else
			xml.skipCurrentElement();
	}
	
	// Sanity checks
	if (options.scale <= 0)
		options.scale = map.getScaleDenominator();
	if (options.resolution <= 0)
		options.resolution = 600;
}

void MapPrinterConfig::save(QXmlStreamWriter& xml, const QString& element_name) const
{
	ScopedXmlWriterElement printer_config_element(xml, element_name);
	
	printer_config_element.writeAttribute("scale", options.scale);
	printer_config_element.writeAttribute("resolution", options.resolution);
	printer_config_element.writeAttribute("templates_visible", options.show_templates);
	printer_config_element.writeAttribute("grid_visible", options.show_grid);
	printer_config_element.writeAttribute("simulate_overprinting", options.simulate_overprinting);
	
	{
		ScopedXmlWriterElement page_format_element(xml, "page_format");
		page_format_element.writeAttribute("paper_size",
		  MapPrinter::paperSizeNames()[page_format.paper_size]);
		page_format_element.writeAttribute("orientation",
		  (page_format.orientation == QPrinter::Portrait) ? "portrait" : "landscape" );
		page_format_element.writeAttribute("h_overlap", page_format.h_overlap, 2);
		page_format_element.writeAttribute("v_overlap", page_format.v_overlap, 2);
		{
			ScopedXmlWriterElement(xml, "dimensions").write(page_format.paper_dimensions);
		}
		{
			ScopedXmlWriterElement(xml, "page_rect").write(page_format.page_rect);
		}
	}
	{
		ScopedXmlWriterElement print_area_element(xml, "print_area");
		print_area_element.write(print_area);
		print_area_element.writeAttribute("center_area", center_print_area);
		print_area_element.writeAttribute("single_page", single_page_print_area);
	}
}



// ### MapPrinter ###

const QPrinterInfo* MapPrinter::pdfTarget()
{
	static QPrinterInfo pdf_target; // TODO: set name and features?
	return &pdf_target;
}

const QPrinterInfo* MapPrinter::imageTarget()
{
	static QPrinterInfo image_target; // TODO: set name and features?
	return &image_target;
}

const QHash< int, const char* >& MapPrinter::paperSizeNames()
{
	static QHash< int, const char* > names;
	if (names.empty())
	{
		names[QPrinter::A0]        = "A0";
		names[QPrinter::A1]        = "A1";
		names[QPrinter::A2]        = "A2";
		names[QPrinter::A3]        = "A3";
		names[QPrinter::A4]        = "A4";
		names[QPrinter::A5]        = "A5";
		names[QPrinter::A6]        = "A6";
		names[QPrinter::A7]        = "A7";
		names[QPrinter::A8]        = "A8";
		names[QPrinter::A9]        = "A9";
		names[QPrinter::B0]        = "B0";
		names[QPrinter::B1]        = "B1";
		names[QPrinter::B2]        = "B2";
		names[QPrinter::B3]        = "B3";
		names[QPrinter::B4]        = "B4";
		names[QPrinter::B5]        = "B5";
		names[QPrinter::B6]        = "B6";
		names[QPrinter::B7]        = "B7";
		names[QPrinter::B8]        = "B8";
		names[QPrinter::B9]        = "B9";
		names[QPrinter::B10]       = "B10";
		names[QPrinter::C5E]       = "C5E";
		names[QPrinter::DLE]       = "DLE";
		names[QPrinter::Executive] = "Executive";
		names[QPrinter::Folio]     = "Folio";
		names[QPrinter::Ledger]    = "Ledger";
		names[QPrinter::Legal]     = "Legal";
		names[QPrinter::Letter]    = "Letter";
		names[QPrinter::Tabloid]   = "Tabloid";
		names[QPrinter::Comm10E]   = "US Common #10 Envelope";
		names[QPrinter::Custom]    = "Custom";
	}
	return names;
}


MapPrinter::MapPrinter(Map& map, QObject* parent)
: QObject(parent),
  MapPrinterConfig(map.printerConfig()),
  map(map),
  view(NULL),
  target(NULL)
{
	scale_adjustment = map.getScaleDenominator() / (qreal) options.scale;
	updatePaperDimensions();
}

MapPrinter::~MapPrinter()
{
	// Do not remove.
}

void MapPrinter::saveConfig() const
{
	map.setPrinterConfig(*this);
}

// slot
void MapPrinter::setTarget(const QPrinterInfo* new_target)
{
	if (new_target != target)
	{
		const QPrinterInfo* old_target = target;
		if (new_target == NULL)
			target = new_target;
		else if (new_target == pdfTarget())
			target = new_target;
		else if (new_target == imageTarget())
			target = new_target;
		else
		{
			// We don't own this target, so we need to make a copy.
			target_copy = *new_target;
			target = &target_copy;
		}
		emit targetChanged(target);
		
		if (old_target == imageTarget() || new_target == imageTarget())
		{
			// No page margins. Will emit pageFormatChanged( ).
			setCustomPaperSize(page_format.page_rect.size());
		}
	}
}

QPrinter* MapPrinter::makePrinter() const
{
	QPrinter* printer = (target==NULL) ? 
	  new QPrinter(QPrinter::HighResolution) : new QPrinter(*target, QPrinter::HighResolution);
	
	printer->setDocName("MAP"/* FIXME QFileInfo(main_window->getCurrentFilePath()).fileName()*/);
	printer->setFullPage(true);
	if (page_format.paper_size == QPrinter::Custom)
	{
		// TODO: Proper handling of custom paper dimensions.
		printer->setPaperSize(print_area.size() * scale_adjustment, QPrinter::Millimeter);
		printer->setOrientation(QPrinter::Portrait);
	}
	else
	{
		printer->setPaperSize(page_format.paper_size);
		printer->setOrientation(page_format.orientation);
	}
	printer->setColorMode(QPrinter::Color);
	printer->setResolution(options.resolution);
	
	if (target == imageTarget() || page_format.paper_size == QPrinter::Custom)
		printer->setPageMargins(0.0, 0.0, 0.0, 0.0, QPrinter::Millimeter);
	
	return printer;
}

bool MapPrinter::isPrinter() const
{
	if (target == NULL)
		return false;
	else if (target == pdfTarget())
		return false;
	else if (target == imageTarget())
		return false;
	
	return true;
}

// slot
void MapPrinter::setPrintArea(const QRectF& area)
{
	if (print_area != area && area.left() < area.right() && area.top() < area.bottom())
	{
		print_area = area;
		
		if (target == imageTarget() && print_area.size() != page_format.paper_dimensions)
			setCustomPaperSize(print_area.size() * scale_adjustment);
		
		updatePageBreaks();
		
		emit printAreaChanged(print_area);
	}
}

// slot
void MapPrinter::setPaperSize(const QPrinter::PaperSize size)
{
	if (size == QPrinter::Custom)
		setCustomPaperSize(print_area.size() * scale_adjustment);
	else if (page_format.paper_size != size)
	{
		if ( page_format.paper_size == QPrinter::Custom &&
		     page_format.paper_dimensions.width() > page_format.paper_dimensions.height() )
		{
			// After QPrinter::Custom, determine orientation from actual dimensions.
			page_format.orientation = QPrinter::Landscape;
		}
		page_format.paper_size = size;
		updatePaperDimensions();
		emit pageFormatChanged(page_format);
	}
}

// slot
void MapPrinter::setCustomPaperSize(const QSizeF dimensions)
{
	if ((page_format.paper_size != QPrinter::Custom || 
	     page_format.paper_dimensions != dimensions) &&
	     ! dimensions.isEmpty())
	{
		page_format.paper_size = QPrinter::Custom;
		page_format.orientation = QPrinter::Portrait;
		page_format.paper_dimensions = dimensions;
		updatePaperDimensions();
		emit pageFormatChanged(page_format);
	}
}

// slot
void MapPrinter::setPageOrientation(const QPrinter::Orientation orientation)
{
	if (page_format.paper_size == QPrinter::Custom)
	{
		// do nothing
		emit pageFormatChanged(page_format);
	}
	else if (page_format.orientation != orientation)
	{
		page_format.orientation = orientation;
		page_format.paper_dimensions.transpose();
		updatePaperDimensions();
		updatePageBreaks();
		emit pageFormatChanged(page_format);
	}
}

// slot
void MapPrinter::setOverlap(qreal h_overlap, qreal v_overlap)
{
	if (page_format.h_overlap != h_overlap || page_format.v_overlap != v_overlap)
	{
		page_format.h_overlap = qMax(0.0, qMin(h_overlap, page_format.page_rect.width()));
		page_format.v_overlap = qMax(0.0, qMin(v_overlap, page_format.page_rect.height()));
		updatePageBreaks();
		emit pageFormatChanged(page_format);
	}
}

void MapPrinter::updatePaperDimensions()
{
	if (target == imageTarget() && page_format.paper_size == QPrinter::Custom)
	{
		// No margins, no need to query QPrinter.
		page_format.page_rect = QRectF(QPointF(0.0, 0.0), page_format.paper_dimensions);
		page_format.h_overlap = qMax(0.0, qMin(page_format.h_overlap, page_format.page_rect.width()));
		page_format.v_overlap = qMax(0.0, qMin(page_format.v_overlap, page_format.page_rect.height()));
		updatePageBreaks();
		return;
	}
	
	QPrinter* printer = (target==NULL) ? 
	  new QPrinter(QPrinter::HighResolution) : new QPrinter(*target, QPrinter::HighResolution);
	  
	if (page_format.paper_size == QPrinter::Custom)
	{
		printer->setPaperSize(page_format.paper_dimensions, QPrinter::Millimeter);
		page_format.orientation = printer->orientation(); 
	}
	else
	{
		printer->setPaperSize(page_format.paper_size);
		printer->setOrientation(page_format.orientation);
	}
	
	page_format.page_rect = printer->paperRect(QPrinter::Millimeter);
	page_format.paper_dimensions = page_format.page_rect.size();
	
	if (target != imageTarget() && page_format.paper_size != QPrinter::Custom)
	{
		qreal left, top, right, bottom;
		printer->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
		page_format.page_rect.adjust(left, top, -right, -bottom);
	}
	page_format.h_overlap = qMax(0.0, qMin(page_format.h_overlap, page_format.page_rect.width()));
	page_format.v_overlap = qMax(0.0, qMin(page_format.v_overlap, page_format.page_rect.height()));
	
	delete printer;
	updatePageBreaks();
}

// slot
void MapPrinter::setResolution(const unsigned int dpi)
{
	Q_ASSERT(dpi >= 0.05f);
	if (options.resolution != dpi)
	{
		options.resolution = dpi;
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setScale(const unsigned int value)
{
	if (options.scale != value)
	{
		Q_ASSERT(value > 0);
		options.scale = value;
		scale_adjustment = map.getScaleDenominator() / (qreal) value;
		updatePageBreaks();
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setPrintTemplates(const bool visible, MapView* view)
{
	if (options.show_templates != visible || this->view != view)
	{
		options.show_templates = visible;
		this->view = visible ? view : NULL;
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setPrintGrid(const bool visible)
{
	if (options.show_grid != visible)
	{
		options.show_grid = visible;
		emit optionsChanged(options);
	}
}

// slot
void MapPrinter::setSimulateOverprinting(const bool enabled)
{
	if (options.simulate_overprinting != enabled)
	{
		options.simulate_overprinting = enabled;
		emit optionsChanged(options);
	}
}

bool MapPrinter::isOutputEmpty() const
{
	return (
	  (map.getNumObjects() == 0 || (view != NULL && (view->getMapVisibility()->visible == false || view->getMapVisibility()->opacity < 0.0005f))) &&
	  (!options.show_templates || map.getNumTemplates() == 0) &&
	  !options.show_grid
	);
}

void MapPrinter::updatePageBreaks()
{
	Q_ASSERT(print_area.left() <= print_area.right());
	Q_ASSERT(print_area.top() <= print_area.bottom());
	
	// This whole implementation needs to deal with FP precision issues
	
	h_page_pos.clear();
	qreal h_pos = print_area.left();
	h_page_pos.push_back(h_pos);
	const qreal h_overlap = page_format.h_overlap / scale_adjustment;
	const qreal page_width = page_format.page_rect.width() / scale_adjustment - h_overlap;
	const qreal right_bound = print_area.right() - h_overlap - 0.05;
	if (page_width >= 0.01)
	{
		for (h_pos += page_width; h_pos < right_bound; h_pos += page_width)
			h_page_pos.push_back(h_pos);
		
		// Center the print area on the pages total area.
		// Don' pre-calculate this offset to avoid FP precision problems
		const qreal h_offset = 0.5 * (h_pos + h_overlap - print_area.right());
		for (std::vector<qreal>::iterator it=h_page_pos.begin(); it != h_page_pos.end(); ++it)
			*it -= h_offset;
	}
	
	v_page_pos.clear();
	qreal v_pos = print_area.top();
	v_page_pos.push_back(v_pos);
	const qreal v_overlap = page_format.v_overlap / scale_adjustment;
	const qreal page_height = page_format.page_rect.height() / scale_adjustment - v_overlap;
	const qreal bottom_bound = print_area.bottom() - v_overlap - 0.05;
	if (page_height >= 0.01)
	{
		for (v_pos += page_height; v_pos < bottom_bound; v_pos += page_height)
			v_page_pos.push_back(v_pos);
		
		// Don' pre-calculate offset to avoid FP precision problems
		const qreal v_offset = 0.5 * (v_pos + v_overlap - print_area.bottom());
		for (std::vector<qreal>::iterator it=v_page_pos.begin(); it != v_page_pos.end(); ++it)
			*it -= v_offset;
	}
}

void MapPrinter::takePrinterSettings(const QPrinter* printer)
{
	MapPrinterPageFormat f;
	f.paper_size  = printer->paperSize();
	f.orientation = printer->orientation();
	f.page_rect   = printer->paperRect(QPrinter::Millimeter); // temporary
	f.paper_dimensions = f.page_rect.size();
	
	qreal left, top, right, bottom;
	printer->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
	f.page_rect.adjust(left, top, -right, -bottom);
	
	if (f != page_format)
	{
		page_format.paper_size = f.paper_size;
		page_format.orientation = f.orientation;
		page_format.paper_dimensions = f.paper_dimensions;
		page_format.page_rect = f.page_rect;
		updatePageBreaks();
		emit pageFormatChanged(page_format);
	}
}

void MapPrinter::drawPage(QPainter* device_painter, float dpi, const QRectF& page_extent, bool white_background) const
{
	device_painter->save();
	
	device_painter->setRenderHint(QPainter::Antialiasing);
	device_painter->setRenderHint(QPainter::SmoothPixmapTransform);
	
	// Painting transparently is not supported for print devices.
	// If there are any transparent features, use a temporary image 
	// which is drawn on the device printer as last step.
	// If the target is an image, use the temporary image to enforce
	// the given resolution during preview.
	bool have_transparency = options.simulate_overprinting || target == imageTarget();
	if (view != NULL)
	{
		have_transparency = view->getMapVisibility()->visible && view->getMapVisibility()->opacity < 1.0f;
		for (int i = 0; i < map.getNumTemplates() && !have_transparency; ++i)
		{
			TemplateVisibility* visibility = view->getTemplateVisibility(map.getTemplate(i));
			have_transparency = visibility->visible && visibility->opacity < 1.0f;
		}
	}
	
	// Dots per mm
	qreal scale = dpi / 25.4;
	
	QPainter* painter = device_painter;
	QImage print_buffer;
	if (have_transparency)
	{
		int w = qCeil(device_painter->device()->widthMM() * scale);
		int h = qCeil(device_painter->device()->heightMM() * scale);
		print_buffer = QImage(w, h, QImage::Format_RGB32);
		print_buffer.fill(QColor(Qt::white));
		painter = new QPainter(&print_buffer);
		painter->setRenderHints(device_painter->renderHints());
	}
	else if (white_background)
		painter->fillRect(QRect(0, 0, painter->device()->width(), painter->device()->height()), Qt::white);
	
	// Translate for top left page margin 
	painter->scale(scale, scale);
	painter->translate(page_format.page_rect.left(), page_format.page_rect.top());
	
	// Convert native map scale to print scale
	if (scale_adjustment != 1.0)
	{
		scale *= scale_adjustment;
		painter->scale(scale_adjustment, scale_adjustment);
	}
	
	// Translate and clip for margins and print area
	painter->translate(-page_extent.left(), -page_extent.top());
	painter->setClipRect(page_extent.intersected(print_area), Qt::ReplaceClip);
	
	if (options.show_templates)
		map.drawTemplates(painter, page_extent, 0, map.getFirstFrontTemplate() - 1, view, false);
	
	if (view == NULL || view->getMapVisibility()->visible)
	{
		QImage map_buffer;
		QPainter* map_painter = painter;
		if (have_transparency)
		{
			// Draw map into a temporary buffer first which is printed with the map's opacity later.
			// This prevents artifacts with overlapping objects.
			map_buffer = QImage(print_buffer.size(), QImage::Format_ARGB32_Premultiplied);
			map_buffer.fill(QColor(Qt::transparent));
			
			map_painter = new QPainter(&map_buffer);
			map_painter->setRenderHints(painter->renderHints());
			map_painter->setTransform(painter->transform());
		}
		
		if (options.simulate_overprinting)
			map.drawOverprintingSimulation(map_painter, page_extent, false, scale, false, false);
		else
			map.draw(map_painter, page_extent, false, scale, false, false);
			
		if (map_painter != painter)
		{
			delete map_painter;
			map_painter = NULL;
			
			// Print buffer with map opacity
			painter->save();
			painter->resetTransform();
			if (view != NULL)
				painter->setOpacity(view->getMapVisibility()->opacity);
			painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
			painter->drawImage(0, 0, map_buffer);
			painter->restore();
		}
	}
	
	if (options.show_grid)
		map.drawGrid(painter, print_area);
	
	if (options.show_templates)
		map.drawTemplates(painter, page_extent, map.getFirstFrontTemplate(), map.getNumTemplates() - 1, view, false);
	
	// If a temporary buffer has been used, paint it on the device painter
	if (painter != device_painter)
	{
		delete painter; 
		painter = NULL;
		
		device_painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
		device_painter->drawImage(0, 0, print_buffer);
	}
	
	device_painter->restore();
}

// slot
void MapPrinter::printMap(QPrinter* printer)
{
	// Printer settings may have been changed by preview or application.
	// We need to use them for printing.
	takePrinterSettings(printer);
	
	QSizeF extent_size = page_format.page_rect.size() / scale_adjustment;
	QPainter p(printer);
	bool need_new_page = false;
	Q_FOREACH(qreal vpos, v_page_pos)
	{
		Q_FOREACH(qreal hpos, h_page_pos)
		{
			if (p.isActive())
			{
				if (need_new_page)
					printer->newPage();
				
				QRectF page_extent = QRectF(QPointF(hpos, vpos), extent_size);
				drawPage(&p, (float)options.resolution, page_extent, false);
				need_new_page = true;
			}
		}
	}
}
