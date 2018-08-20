// Copyright 2018, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#pragma once

#include "CoreMinimal.h"
#include "OwlStructs.h"

/**
* Owl/Xml node
* node will not have Value and Children
*/
struct FOwlNode
{
public:
	// Node prefixed name
	FOwlPrefixName Name;

	// Node value
	FString Value;

	// Attributes
	TArray<FOwlAttribute> Attributes;

	// Nodes
	TArray<FOwlNode> ChildNodes;

	// Comment
	FString Comment;

public:
	// Default constructor (emtpy node)
	FOwlNode() {}

	// Init constructor, NO Value, Attributes or Children
	FOwlNode(const FOwlPrefixName& InName) : Name(InName) {}

	// Init constructor, NO Value or Attributes 
	FOwlNode(const FOwlPrefixName& InName,
		const TArray<FOwlNode>& InChildNodes) :
		Name(InName),
		ChildNodes(InChildNodes)
	{}

	// Init constructor, NO Value and Children
	FOwlNode(const FOwlPrefixName& InName,
		const TArray<FOwlAttribute>& InAttributes) :
		Name(InName),
		Attributes(InAttributes)
	{}

	// Init constructor, NO Value and Children, one attribute
	FOwlNode(const FOwlPrefixName& InName,
		const FOwlAttribute& InAttribute) :
		Name(InName)
	{
		Attributes.Add(InAttribute);
	}

	// Init constructor, NO Value
	FOwlNode(const FOwlPrefixName& InName,
		const TArray<FOwlAttribute>& InAttributes,
		const TArray<FOwlNode>& InChildNodes) :
		Name(InName),
		Attributes(InAttributes),
		ChildNodes(InChildNodes)
	{}

	// Init constructor, NO Value, one attribute
	FOwlNode(const FOwlPrefixName& InName,
		const FOwlAttribute& InAttribute,
		const TArray<FOwlNode>& InChildNodes) :
		Name(InName),
		ChildNodes(InChildNodes)
	{
		Attributes.Add(InAttribute);
	}

	// Init constructor, NO Children
	FOwlNode(const FOwlPrefixName& InName,
		const TArray<FOwlAttribute>& InAttributes,
		const FString& InValue) :
		Name(InName),
		Attributes(InAttributes),
		Value(InValue)
	{}

	// Init constructor, NO Children, one attribute
	FOwlNode(const FOwlPrefixName& InName,
		const FOwlAttribute& InAttribute,
		const FString& InValue) :
		Name(InName),
		Value(InValue)
	{
		Attributes.Add(InAttribute);
	}

	// Add child node
	void AddChildNode(const FOwlNode& InChildNode)
	{
		ChildNodes.Add(InChildNode);
	}

	// Add child nodes
	void AddChildNodes(const TArray<FOwlNode>& InChildNodes)
	{
		ChildNodes.Append(InChildNodes);
	}

	// Add attribute
	void AddAttribute(const FOwlAttribute& InAttribute)
	{
		Attributes.Add(InAttribute);
	}

	// Add attributes
	void AddAttributes(const TArray<FOwlAttribute>& InAttributes)
	{
		Attributes.Append(InAttributes);
	}

	// True if all data is empty
	bool IsEmpty() const
	{
		return Name.IsEmpty() &&
			Value.IsEmpty() &&
			Attributes.Num() == 0 &&
			ChildNodes.Num() == 0 &&
			Comment.IsEmpty();
	}

	// Clear all data
	void Emtpy()
	{
		Name.Empty();
		Value.Empty();
		Attributes.Empty();
		ChildNodes.Empty();
		Comment.Empty();
	}

	// Destructor
	~FOwlNode() {}

	// Return node as string
	FString ToString(FString& Indent)
	{
		FString NodeStr;
		// Add comment
		if (!Comment.IsEmpty())
		{
			NodeStr += TEXT("\n") + Indent + TEXT("<!-- ") + Comment + TEXT(" -->\n");
		}

		// Comment only OR empty node
		if (Name.ToString().IsEmpty())
		{
			return NodeStr;
		}

		// Add node name
		NodeStr += Indent + TEXT("<") + Name.ToString();

		// Add attributes to tag
		for (int32 i = 0; i < Attributes.Num(); ++i)
		{
			if (Attributes.Num() == 1)
			{
				NodeStr += TEXT(" ") + Attributes[i].ToString();
			}
			else
			{
				if (i < (Attributes.Num() - 1))
				{
					NodeStr += TEXT(" ") + Attributes[i].ToString() + TEXT("\n") + Indent + INDENT_STEP;
				}
				else
				{
					// Last attribute does not have new line
					NodeStr += TEXT(" ") + Attributes[i].ToString();
				}
			}
		}

		// Check node data (children/value)
		bool bHasChildren = ChildNodes.Num() != 0;
		bool bHasValue = !Value.IsEmpty();
			
		// Node cannot have value and children
		if (!bHasChildren && !bHasValue)
		{
			// No children nor value, close tag
			NodeStr += TEXT("/>\n");
		}
		else if (bHasValue)
		{
			// Node has a value, add value
			NodeStr += TEXT(">") + Value + TEXT("</") + Name.ToString() + TEXT(">\n");
		}
		else if(bHasChildren)
		{
			// Node has children, add children
			NodeStr += TEXT(">\n");
			
			// Increase indentation
			Indent += INDENT_STEP;
			
			// Iterate children and add nodes
			for (auto& ChildItr : ChildNodes)
			{
				NodeStr += ChildItr.ToString(Indent);
			}
			
			// Decrease indentation
			Indent.RemoveFromEnd(INDENT_STEP);
			
			// Close tag
			NodeStr += Indent + Value + TEXT("</") + Name.ToString() + TEXT(">\n");
		}
		return NodeStr;
	}
};

// Overloads the constructor to set directly the comment
struct FOwlCommentNode : public FOwlNode
{
	// Default constructor
	FOwlCommentNode(const FString& InComment)
	{
		Comment = InComment;
	}
};