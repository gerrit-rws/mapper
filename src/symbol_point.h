/*
 *    Copyright 2012, 2013 Thomas Schöps
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


#ifndef _OPENORIENTEERING_SYMBOL_POINT_H_
#define _OPENORIENTEERING_SYMBOL_POINT_H_

#include "symbol.h"
#include "symbol_properties_widget.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

class Map;
class MapColor;
class MapEditorController;
class SymbolSettingDialog;
class PointSymbolEditorWidget;
class ColorDropDown;

class PointSymbol : public Symbol
{
friend class PointSymbolSettings;
friend class PointSymbolEditorWidget;
friend class OCAD8FileImport;
friend class XMLImportExport;
public:
	/// Constructs an empty point symbol
	PointSymbol();
	virtual ~PointSymbol();
	virtual Symbol* duplicate(const MapColorMap* color_map = NULL) const;
	
	virtual void createRenderables(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output);
	void createRenderablesScaled(Object* object, const MapCoordVector& flags, const MapCoordVectorF& coords, ObjectRenderables& output, float coord_scale);
	virtual void colorDeleted(const MapColor* color);
	virtual bool containsColor(const MapColor* color) const;
	const MapColor* getDominantColorGuess() const;
	virtual void scale(double factor);
	
	// Contained objects and symbols (elements)
	int getNumElements() const;
	void addElement(int pos, Object* object, Symbol* symbol);
	Object* getElementObject(int pos);
	const Object* getElementObject(int pos) const;
	Symbol* getElementSymbol(int pos);
	const Symbol* getElementSymbol(int pos) const;
	void deleteElement(int pos);
	
	/// Returns true if the point contains no elements and does not create renderables itself
	bool isEmpty() const;
	
	/// Checks if the contained elements are rotationally symmetrical around the origin (this means, only point elements at (0,0) are allowed)
	bool isSymmetrical() const;
	
	// Getters / Setters
	inline bool isRotatable() const {return rotatable;}
	inline void setRotatable(bool enable) {rotatable = enable;}
	inline int getInnerRadius() const {return inner_radius;}
	inline void setInnerRadius(int value) {inner_radius = value;}
	inline const MapColor* getInnerColor() const {return inner_color;}
	inline void setInnerColor(const MapColor* color) {inner_color = color;}
	inline int getOuterWidth() const {return outer_width;}
	inline void setOuterWidth(int value) {outer_width = value;}
	inline const MapColor* getOuterColor() const {return outer_color;}
	inline void setOuterColor(const MapColor* color) {outer_color = color;}
	
	virtual SymbolPropertiesWidget* createPropertiesWidget(SymbolSettingDialog* dialog);
	
	
protected:
	virtual void saveImpl(QIODevice* file, Map* map);
	virtual bool loadImpl(QIODevice* file, int version, Map* map);
	virtual void saveImpl(QXmlStreamWriter& xml, const Map& map) const;
	virtual bool loadImpl(QXmlStreamReader& xml, Map& map, SymbolDictionary& symbol_dict);
	virtual bool equalsImpl(Symbol* other, Qt::CaseSensitivity case_sensitivity);
	
	std::vector<Object*> objects;
	std::vector<Symbol*> symbols;
	
	bool rotatable;
	int inner_radius;		// in 1/1000 mm
	const MapColor* inner_color;
	int outer_width;		// in 1/1000 mm
	const MapColor* outer_color;
};



class PointSymbolSettings : public SymbolPropertiesWidget
{
Q_OBJECT
public:
	PointSymbolSettings(PointSymbol* symbol, SymbolSettingDialog* dialog);
	
	virtual void reset(Symbol* symbol);
	
public slots:
	void tabChanged(int index);
	
private:
	PointSymbol* symbol;
	PointSymbolEditorWidget* symbol_editor;
	QVBoxLayout* layout;
	QWidget* point_tab;
	
};

#endif